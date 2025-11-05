#include "core/soir.hh"

#include "absl/log/log.h"
#include "absl/status/status.h"

namespace soir {

absl::Status Soir::Init(const std::string& config_path) {
  if (initialized_) {
    return absl::FailedPreconditionError("Soir already initialized");
  }

  auto config_or = utils::Config::FromPath(config_path);
  if (!config_or.ok()) {
    return config_or.status();
  }

  config_ = std::make_unique<utils::Config>(std::move(config_or.value()));
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
