#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <thread>

#include "neon.grpc.pb.h"
#include "utils/config.hh"

namespace neon {
namespace agent {

// Subscribes to notifications from Neon.
class Subscriber {
 public:
  Subscriber();
  ~Subscriber();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

 private:
  std::string user_;
  std::thread thread_;

  std::string neon_grpc_host_;
  int neon_grpc_port_;
  std::unique_ptr<proto::Neon::Stub> neon_stub_;
  grpc::ClientContext context_;
};

}  // namespace agent
}  // namespace neon
