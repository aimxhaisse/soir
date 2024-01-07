#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <memory>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace matin {

class ControllerWatcher {
 public:
  ControllerWatcher();
  ~ControllerWatcher();

  absl::Status Init(const common::Config& config, proto::Midi::Stub* stub);
  absl::Status Start();
  absl::Status Stop();

 private:
  proto::Midi::Stub* midi_stub_;
  std::unique_ptr<libremidi::midi_in> midi_in_;
  std::unique_ptr<libremidi::observer> midi_obs_;
};

}  // namespace matin
}  // namespace maethstro
