#include "core/soir.hh"

#include "absl/log/log.h"
#include "absl/status/status.h"

namespace soir {

absl::Status Soir::Init(utils::Config* config) {
  if (initialized_) {
    return absl::FailedPreconditionError("Soir already initialized");
  }

  config_ = config;
  initialized_ = true;

  LOG(INFO) << "Soir initialized";
  return absl::OkStatus();
}

absl::Status Soir::Start() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Soir not initialized");
  }

  if (running_) {
    return absl::FailedPreconditionError("Soir already running");
  }

  running_ = true;

  LOG(INFO) << "Soir started";
  return absl::OkStatus();
}

absl::Status Soir::Stop() {
  if (!running_) {
    return absl::FailedPreconditionError("Soir not running");
  }

  running_ = false;

  LOG(INFO) << "Soir stopped";
  return absl::OkStatus();
}

absl::Status Soir::UpdateCode(const std::string& code) {
  return absl::OkStatus();
}

}  // namespace soir
