#include <absl/log/log.h>
#include <absl/time/clock.h>
#include <pybind11/embed.h>

#include "engine.hh"

namespace maethstro {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const Config& config) {
  LOG(INFO) << "Initializing engine";

  running_ = true;

  return absl::OkStatus();
}

absl::Status Engine::Run() {
  py::scoped_interpreter guard;

  while (true) {
    std::list<std::string> updates;
    {
      std::unique_lock<std::mutex> lock(loop_mutex_);
      loop_cv_.wait(lock,
                    [this] { return !code_updates_.empty() || !running_; });
      if (!running_) {
        LOG(INFO) << "Received stop signal";
        break;
      }
      std::swap(updates, code_updates_);
    }

    for (const auto& code : updates) {
      try {
        py::exec(code.c_str(), py::globals(), py::globals());
      } catch (py::error_already_set& e) {
        LOG(ERROR) << "Python error: " << e.what();
      }
    }
  }

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    running_ = false;
    loop_cv_.notify_all();
  }

  return absl::OkStatus();
}

absl::Status Engine::Wait() {
  return absl::OkStatus();
}

absl::Status Engine::UpdateCode(const std::string& code) {
  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    code_updates_.push_back(code);
    loop_cv_.notify_all();
  }

  LOG(INFO) << "Code update queued";

  return absl::OkStatus();
}

absl::Status Engine::OnBeat() {
  LOG(INFO) << "OnBeat";
  return absl::OkStatus();
}

}  // namespace maethstro
