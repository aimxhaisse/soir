#include <absl/log/log.h>
#include <libremidi/libremidi.hpp>

#include "controller_watcher.hh"

namespace maethstro {
namespace matin {

ControllerWatcher::ControllerWatcher() {}

ControllerWatcher::~ControllerWatcher() {}

absl::Status ControllerWatcher::Init(const common::Config& config,
                                     proto::Midi::Stub* stub) {
  midi_stub_ = stub;

  return absl::OkStatus();
}

absl::Status ControllerWatcher::Start() {
  return absl::OkStatus();
}

absl::Status ControllerWatcher::Stop() {
  return absl::OkStatus();
}

}  // namespace matin
}  // namespace maethstro
