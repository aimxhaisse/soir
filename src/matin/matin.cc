#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <grpc++/grpc++.h>
#include <filesystem>
#include <regex>
#include <vector>

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

absl::Status Matin::Run() {
  auto status = file_watcher_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start file watcher: " << status.message();
    return status;
  }

  std::thread subscriber_thread([this]() {
    auto status = subscriber_->Run();
    if (!status.ok()) {
      LOG(ERROR) << "Subscriber failed: " << status.message();
    }
  });

  LOG(INFO) << "Matin running";

  subscriber_thread.join();

  return absl::OkStatus();
}

absl::Status Matin::Stop() {
  LOG(INFO) << "Matin stopping";

  auto status = subscriber_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Subscriber failed to stop: " << status.message();
  }

  status = file_watcher_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "File watcher failed to stop: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Matin::Wait() {
  LOG(INFO) << "Matin properly shut down";

  auto status = subscriber_->Wait();
  if (!status.ok()) {
    LOG(ERROR) << "Subscriber failed to wait: " << status.message();
  }

  return absl::OkStatus();
}

}  // namespace matin
}  // namespace maethstro
