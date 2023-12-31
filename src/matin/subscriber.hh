#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <thread>

#include "common/config.hh"
#include "live.grpc.pb.h"
#include "utils.hh"

namespace maethstro {
namespace matin {

// Subscribes to notifications from Midi.
class Subscriber {
 public:
  Subscriber();
  ~Subscriber();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

 private:
  std::string user_;
  std::thread thread_;

  std::string midi_grpc_host_;
  int midi_grpc_port_;
  std::unique_ptr<proto::Midi::Stub> midi_stub_;
  grpc::ClientContext context_;
};

}  // namespace matin
}  // namespace maethstro
