#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <memory>

#include "neon.grpc.pb.h"
#include "utils/config.hh"

namespace neon {
namespace agent {

class ControllerWatcher {
 public:
  ControllerWatcher();
  ~ControllerWatcher();

  absl::Status Init(const utils::Config& config, proto::Neon::Stub* stub);
  absl::Status Start();
  absl::Status Stop();

 private:
  proto::Neon::Stub* neon_stub_;
  std::unique_ptr<libremidi::midi_in> midi_in_;
  std::unique_ptr<libremidi::observer> midi_obs_;
};

}  // namespace agent
}  // namespace neon
