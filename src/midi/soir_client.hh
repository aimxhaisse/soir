#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace midi {

// Handles communications to Soir in a dedicated thread.
class SoirClient {
 public:
  SoirClient();
  ~SoirClient();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status SendMidiEvent(const proto::MidiEvents_Request& event);
  absl::Status GetTracks(proto::GetTracks_Response* response);
  absl::Status SetupTracks(const proto::SetupTracks_Request& request,
                           proto::SetupTracks_Response* response);

 private:
  absl::Status Run();

  std::string grpc_host_;
  int grpc_port_;

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool running_ = false;
  std::list<proto::MidiEvents_Request> events_;

  std::unique_ptr<proto::Soir::Stub> soir_stub_;
};

}  // namespace midi
}  // namespace maethstro
