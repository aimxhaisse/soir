#include <absl/log/log.h>
#include <filesystem>

#include "common/utils.hh"
#include "matin.hh"

namespace maethstro {
namespace matin {

Matin::Matin() {}

Matin::~Matin() {}

absl::Status Matin::Init(const Config& config) {
  file_watcher_ = std::make_unique<FileWatcher>();
  auto status = file_watcher_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize file watcher: " << status.message();
    return status;
  }

  subscriber_ = std::make_unique<matin::Subscriber>();
  status = subscriber_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize subscriber: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Matin::Start() {
  auto status = file_watcher_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start file watcher: " << status.message();
    return status;
  }

  status = subscriber_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start subscriber: " << status.message();
    return status;
  }

  LOG(INFO) << "Matin running";

  return absl::OkStatus();
}

absl::Status Matin::Stop() {
  LOG(INFO) << "Matin stopping";

  auto status = subscriber_->Stop();
  if (!status.ok()) {
    LOG(WARNING) << "Subscriber failed to stop: " << status.message();
  }

  status = file_watcher_->Stop();
  if (!status.ok()) {
    LOG(WARNING) << "File watcher failed to stop: " << status.message();
  }

  LOG(INFO) << "Matin stopped";

  return absl::OkStatus();
}

}  // namespace matin
}  // namespace maethstro
