#ifndef MMA_CLIENT_H
#define MMA_CLIENT_H

#include <memory>
#include <cstdlib>

#include <grpc++/grpc++.h>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif

using mma::Greeter;
#include "array_list.h"
using grpc::Channel;

namespace proj4 {
class ArrayList;

class MmaClient {
public:
    MmaClient(std::shared_ptr<Channel>);
    ArrayList* Allocate(size_t);
    void Free(ArrayList*);
    int Read (int, int, int);
    void Write (int, int, int, int);
    ~MmaClient(){}
private:
  std::unique_ptr<Greeter::Stub> stub_;
  int next_array_id;
};

} //namespace proj4

#endif