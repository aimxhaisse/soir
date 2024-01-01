#include <absl/log/log.h>

#include "engine.hh"

namespace maethstro {
namespace soir {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const common::Config& config) {
  LOG(INFO) << "Initializing engine";

  return absl::OkStatus();
}

absl::Status Engine::Start() {
  LOG(INFO) << "Starting engine";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Engine failed: " << status;
    }
  });

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cv_.notify_all();
  }

  thread_.join();

  LOG(INFO) << "Engine stopped";

  return absl::OkStatus();
}

absl::Status Engine::Run() {
  LOG(INFO) << "Engine running";

  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return stop_; });
    if (stop_) {
      break;
    }
  }

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
