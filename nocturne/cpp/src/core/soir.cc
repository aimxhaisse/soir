#include "core/soir.hh"

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "bindings/rt.hh"
#include "core/engine.hh"
#include "rt/runtime.hh"

namespace soir {

Soir::Soir()
    : dsp_(std::make_unique<Engine>()),
      rt_(std::make_unique<rt::Runtime>()) {}

Soir::~Soir() {
  if (running_) {
    auto status = Stop();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to stop Soir in destructor: " << status;
    }
  }
  rt::bindings::ResetEngines();
}

absl::Status Soir::Init(const std::string& config_path) {
  if (initialized_) {
    return absl::FailedPreconditionError("Soir already initialized");
  }

  LOG(INFO) << "Initializing Soir";

  auto config_or = utils::Config::FromPath(config_path);
  if (!config_or.ok()) {
    return config_or.status();
  }

  config_ = std::make_unique<utils::Config>(std::move(config_or.value()));

  // Initialize DSP engine
  auto status = dsp_->Init(*config_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize DSP engine: " << status;
    return status;
  }

  // Initialize RT engine
  status = rt_->Init(*config_, dsp_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize RT engine: " << status;
    return status;
  }

  // Set up bindings so Python code can access engines
  status = rt::bindings::SetEngines(rt_.get(), dsp_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to set up bindings: " << status;
    return status;
  }

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

  LOG(INFO) << "Starting Soir";

  // Start DSP engine first
  auto status = dsp_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start DSP engine: " << status;
    return status;
  }

  // Then start RT engine
  status = rt_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start RT engine: " << status;
    auto cleanup = dsp_->Stop();  // Clean up
    if (!cleanup.ok()) {
      LOG(ERROR) << "Failed to stop DSP during cleanup: " << cleanup;
    }
    return status;
  }

  running_ = true;

  LOG(INFO) << "Soir started";
  return absl::OkStatus();
}

absl::Status Soir::Stop() {
  if (!running_) {
    return absl::FailedPreconditionError("Soir not running");
  }

  LOG(INFO) << "Stopping Soir";

  // Stop RT engine first
  auto status = rt_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop RT engine: " << status;
  }

  // Then stop DSP engine
  auto dsp_status = dsp_->Stop();
  if (!dsp_status.ok()) {
    LOG(ERROR) << "Failed to stop DSP engine: " << dsp_status;
    if (status.ok()) {
      status = dsp_status;
    }
  }

  running_ = false;

  LOG(INFO) << "Soir stopped";
  return status;
}

absl::Status Soir::UpdateCode(const std::string& code) {
  if (!running_) {
    return absl::FailedPreconditionError("Soir not running");
  }

  return rt_->PushCodeUpdate(code);
}

}  // namespace soir
