/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif
#include "mma_server.h"

namespace proj4 {

std::unique_ptr<Server>* ptr_server;

MemoryManager* mma;

// Logic and data behind the server's behavior.
Status GreeterServiceImpl::Allocate(ServerContext * context, const AllocateRequest* request, AllocateReply* reply) {
  //printf("Start Allocation in mma_server\n");
  int arr = mma->Allocate(request->sz());
  if (arr == -1) {
    //printf("exceed!\n");
    return Status::CANCELLED;
  }
  reply->set_arr(arr);
  //printf("Finish Allocation in mma_server\n");
  return Status::OK;
}
Status GreeterServiceImpl::Free(ServerContext * context, const FreeRequest* request, Empty* reply) {
  mma->Release(request->arr());
  return Status::OK;
}
Status GreeterServiceImpl::Read(ServerContext * context, const ReadRequest* request, ReadReply* reply){
  int res = mma->ReadPage(request->arr(), request->vpid(), request->offset());
  reply->set_result(res);
  return Status::OK;
}
Status GreeterServiceImpl::Write(ServerContext * context, const WriteRequest* request, Empty* reply){
  mma->WritePage(request->arr(), request->vpid(), request->offset(), request->value());
  return Status::OK;
}

void RunServerUL(size_t phy_page_num) {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  ptr_server = &server;
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  mma = new proj4::MemoryManager(phy_page_num,-1);
  server->Wait();
}

void RunServerL(size_t phy_page_num, size_t max_vir_page_num) {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  ptr_server = &server;
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  mma = new proj4::MemoryManager(phy_page_num, max_vir_page_num);
  server->Wait();
}

void ShutdownServer() {
  delete mma;
  (*ptr_server)->Shutdown();
}
}

int main(int argc, char** argv) {
  proj4::RunServerUL(10);

  return 0;
}
