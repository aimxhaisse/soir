#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <thread>

#include "soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {
namespace agent {

// Subscribes to notifications from Soir.
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

  std::string soir_grpc_host_;
  int soir_grpc_port_;
  std::unique_ptr<proto::Soir::Stub> soir_stub_;
  grpc::ClientContext context_;
};

}  // namespace agent
}  // namespace soir
