#include <absl/log/log.h>
#include <absl/time/clock.h>
#include <pybind11/embed.h>

#include "engine.hh"

// PyBind requires modules to be defined in the global scope, as a
// result we need a way to access the engine globally.
maethstro::Engine* gEngine = nullptr;

PYBIND11_EMBEDDED_MODULE(live, m) {
  m.doc() = "Maethstro L I V E module";

  m.def("log", []() {});
  m.def(
      "set_bpm", [](int bpm) { gEngine->Live_SetBPM(bpm); }, "Sets the BPM");
}

namespace maethstro {

Engine::Engine() : notifier_(nullptr) {}

Engine::~Engine() {
  gEngine = nullptr;
}

absl::Status Engine::Init(const Config& config, Notifier* notifier) {
  LOG(INFO) << "Initializing engine";

  notifier_ = notifier;

  Live_SetBPM(config.Get<uint16_t>("midi.initial_bpm"));
  auto status = Beat(absl::Now());
  if (!status.ok()) {
    LOG(ERROR) << "Unable to schedule first beat: " << status;
    return status;
  }

  if (gEngine != nullptr) {
    LOG(ERROR) << "Engine already initialized, unable to run multiple "
                  "instances at the same time";
    return absl::InternalError("Engine already initialized");
  }
  gEngine = this;

  running_ = true;

  return absl::OkStatus();
}

void Engine::Live_SetBPM(uint16_t bpm) {
  LOG(INFO) << "Setting BPM to " << bpm;

  bpm_ = bpm;
  beat_us_ = 60.0 / bpm_ * 1000000;
}

absl::Status Engine::Beat(const absl::Time& now) {
  LOG(INFO) << "Beat " << current_beat_;

  current_beat_ += 1;

  return Schedule(now + absl::Microseconds(beat_us_),
                  [this](const absl::Time& now) { return Beat(now); });
}

absl::Status Engine::Schedule(const absl::Time& at, CallbackFunc func) {
  LOG(INFO) << "Scheduling callback at " << at;

  // This is stupid simple because we currently don't support
  // scheduling callbacks from multiple threads. So it is assumed here
  // we are running in the context of Run(). If we ever support
  // external scheduling, we'll need to wake up the Run loop here in
  // case the next scheduled callback changes.

  callbacks_.insert({at, func});

  return absl::OkStatus();
}

absl::Status Engine::Run() {
  py::scoped_interpreter guard;

  while (true) {
    // We assume there is always at least one callback in the queue
    // due to the beat scheduling.
    auto next = callbacks_.begin();

    std::list<std::string> updates;
    {
      std::unique_lock<std::mutex> lock(loop_mutex_);
      loop_cv_.wait_until(lock, absl::ToChronoTime(next->at), [this, next] {
        return !running_ || !code_updates_.empty() || next->at <= absl::Now();
      });

      if (!running_) {
        LOG(INFO) << "Received stop signal";
        break;
      }

      if (!code_updates_.empty()) {
        std::swap(updates, code_updates_);
      }
    }

    // Process next callback if time has passed.
    if (next->at <= absl::Now()) {
      callbacks_.erase(next);

      // Here we don't use now() as a parameter to the callback to
      // avoid drifts. Instead, we pass the time at which it is
      // expected to be scheduled, so that any drift is corrected on
      // the next scheduling.
      auto status = next->func(next->at);
      if (!status.ok()) {
        LOG(ERROR) << "Callback failed: " << status;
      }
    }

    // Code updates are performed in a second time, after the temporal
    // recursions, to be as precise on time as possible. It's OK if a
    // code update takes 10ms to be applied, but not OK if it's a kick
    // event for example.
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

}  // namespace maethstro
