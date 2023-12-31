#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace matin {

// Subscribes to notifications from Midi.
class Subscriber {
 public:
  Subscriber();
  ~Subscriber();

  absl::Status Init(const Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

 private:
  std::thread thread_;

  std::string midi_grpc_host_;
  int midi_grpc_port_;
  std::unique_ptr<proto::Midi::Stub> midi_stub_;
  grpc::ClientContext context_;

  std::mutex mutex_;
  std::condition_variable cond_;
  bool stopped_ = false;
};

}  // namespace matin
}  // namespace maethstro
