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

#include <grpcpp/grpcpp.h>
#include "mma_client.h"
#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using mma::Greeter;
using mma::AllocateRequest;
using mma::AllocateReply;
using mma::FreeRequest;
using mma::ReadRequest;
using mma::ReadReply;
using mma::WriteRequest;
using mma::Empty;

namespace proj4 {

MmaClient::MmaClient(std::shared_ptr<Channel> channel):stub_(Greeter::NewStub(channel)){
    next_array_id = 0;
}

ArrayList* MmaClient::Allocate(size_t sz) {
  while(true) {
    AllocateRequest request;
    request.set_sz(sz);
    AllocateReply reply;
    ClientContext context;
    //printf("Start Allocation in mma_client\n");
    Status status = stub_->Allocate(&context, request, &reply);
    // Act upon its status.
    if (status.ok()) {
      auto arr = new ArrayList(sz, this, reply.arr());
      return arr;
    } else {
      /*
      if(status.error_code() == 'CANCELLED') {
        printf("Retry Allocation\n");
        continue;
      }*/
      sleep(1);
      //printf("Retry Allocation\n");
      //continue;
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      continue;
      //return NULL;
    }
  }
  //Status status = stub_->Allocate(&context, request, &reply);
  // Act upon its status.
  //if (status.ok()) {
  //  auto arr = new ArrayList(sz, this, reply.arr());
  //  return arr;
  //} else {
  // std::cout << status.error_code() << ": " << status.error_message()
  //            << std::endl;
  //  return NULL;
  //}
}

void MmaClient::Free(ArrayList* arr) {
  FreeRequest request;
  request.set_arr(arr->array_id);
  Empty reply;
  ClientContext context;
  Status status = stub_->Free(&context, request, &reply);
  // Act upon its status.
  if (status.ok()) {
    delete arr;
    return;
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return;
  }
}

int MmaClient::Read (int array_id, int virtual_page_id, int offset) {
  ReadRequest request;
  request.set_arr(array_id);
  request.set_vpid(virtual_page_id);
  request.set_offset(offset);
  ReadReply reply;
  ClientContext context;
  Status status = stub_->Read(&context, request, &reply);
  // Act upon its status.
  if (status.ok()) {
    return reply.result();
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return 0;
  }
}

void MmaClient::Write (int array_id, int virtual_page_id, int offset, int value) {
  WriteRequest request;
  request.set_arr(array_id);
  request.set_vpid(virtual_page_id);
  request.set_offset(offset);
  request.set_value(value);
  Empty reply;
  ClientContext context;
  Status status = stub_->Write(&context, request, &reply);
  // Act upon its status.
  if (status.ok()) {
    return;
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return;
  }
}


// int main(int argc, char** argv) {
//   // Instantiate the client. It requires a channel, out of which the actual RPCs
//   // are created. This channel models a connection to an endpoint specified by
//   // the argument "--target=" which is the only expected argument.
//   // We indicate that the channel isn't authenticated (use of
//   // InsecureChannelCredentials()).
//   std::string target_str;
//   std::string arg_str("--target");
//   if (argc > 1) {
//     std::string arg_val = argv[1];
//     size_t start_pos = arg_val.find(arg_str);
//     if (start_pos != std::string::npos) {
//       start_pos += arg_str.size();
//       if (arg_val[start_pos] == '=') {
//         target_str = arg_val.substr(start_pos + 1);
//       } else {
//         std::cout << "The only correct argument syntax is --target="
//                   << std::endl;
//         return 0;
//       }
//     } else {
//       std::cout << "The only acceptable argument is --target=" << std::endl;
//       return 0;
//     }
//   } else {
//     target_str = "localhost:50051";
//   }
//   MmaClient greeter(
//       grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
//   std::string user("world");
//   std::string reply = greeter.SayHello(user);
//   std::cout << "Greeter received: " << reply << std::endl;

//   return 0;
// }

}