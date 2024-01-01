#include <absl/log/log.h>
#include <absl/time/clock.h>
#include <pybind11/embed.h>

#include "bindings.hh"
#include "engine.hh"
#include "init.hh"

namespace py = pybind11;

namespace maethstro {
namespace midi {

const char kEngineUser[] = "engine";

Engine::Engine() : notifier_(nullptr) {}

Engine::~Engine() {
  bindings::ResetEngine();
}

absl::Status Engine::Init(const common::Config& config, Notifier* notifier) {
  LOG(INFO) << "Initializing engine";

  notifier_ = notifier;
  current_time_ = absl::Now();
  SetBPM(config.Get<uint16_t>("midi.initial_bpm"));

  Beat();

  auto status = bindings::SetEngine(this);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to set engine: " << status;
    return status;
  }

  running_ = true;

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
    std::lock_guard<std::mutex> lock(loop_mutex_);
    running_ = false;
    loop_cv_.notify_all();
  }

  thread_.join();

  LOG(INFO) << "Engine stopped";

  return absl::OkStatus();
}

absl::Time Engine::MicroBeatToTime(MicroBeat beat) const {
  MicroBeat diff_mb = beat - current_beat_;
  uint64_t diff_us = (diff_mb * beat_us_) / 1000000.0;

  return current_time_ + absl::Microseconds(diff_us);
}

uint64_t Engine::MicroBeatToBeat(MicroBeat beat) const {
  return beat / OneBeat;
}

absl::Status Engine::Run() {
  py::scoped_interpreter guard;

  try {
    current_user_ = kEngineUser;
    py::exec(kInitEnginePy);
  } catch (py::error_already_set& e) {
    LOG(ERROR) << "Python error: " << e.what();
    return absl::InternalError("Python error");
  }

  while (true) {
    // We assume there is always at least one callback in the queue
    // due to the beat scheduling.
    auto next = schedule_.begin();
    auto at_time = Engine::MicroBeatToTime(next->at);

    std::list<CodeUpdate> updates;
    {
      std::unique_lock<std::mutex> lock(loop_mutex_);
      loop_cv_.wait_until(lock, absl::ToChronoTime(at_time),
                          [this, next, at_time] {
                            return !running_ || !code_updates_.empty() ||
                                   at_time <= absl::Now();
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
    if (at_time <= absl::Now()) {
      // This is set before the callback is executed so that it can
      // retrieve accurate timing information.
      current_time_ = at_time;
      current_beat_ = next->at;

      try {
        next->func();
      } catch (py::error_already_set& e) {
        LOG(ERROR) << "Python error: " << e.what();
      }

      schedule_.erase(next);
    }

    // Code updates are performed in a second time, after the temporal
    // recursions, to be as precise on time as possible. It's OK if a
    // code update takes 10ms to be applied, but not OK if it's a kick
    // event for example.
    for (const auto& update : updates) {
      current_user_ = update.user;

      try {
        py::exec(update.code.c_str());
      } catch (py::error_already_set& e) {
        LOG(ERROR) << "Python error: " << e.what();
      }
    }
  }

  return absl::OkStatus();
}

float Engine::SetBPM(float bpm) {
  LOG(INFO) << "Setting BPM to " << bpm;

  bpm_ = bpm;
  beat_us_ = 60.0 / bpm_ * 1000000.0;

  return bpm_;
}

float Engine::GetBPM() const {
  return bpm_;
}

MicroBeat Engine::GetCurrentBeat() const {
  return current_beat_;
}

void Engine::Log(const std::string& user, const std::string& message) {
  proto::MidiNotifications_Response notification;

  auto* log = notification.mutable_log();

  log->set_source(user);
  log->set_notification(message);

  auto status = notifier_->Notify(notification);
  if (!status.ok()) {
    LOG(WARNING) << "Unable to send log notification: " << status;
  }
}

std::string Engine::GetUser() const {
  return current_user_;
}

void Engine::Beat() {
  LOG(INFO) << "Beat " << MicroBeatToBeat(current_beat_);

  Schedule(current_beat_ + OneBeat, [this]() { Beat(); });
}

void Engine::Schedule(MicroBeat at, const CbFunc& cb) {
  // This is stupid simple because we currently don't support
  // scheduling callbacks from multiple threads. So it is assumed here
  // we are running in the context of Run(). If we ever support
  // external scheduling, we'll need to wake up the Run loop here in
  // case the next scheduled callback changes.

  schedule_.insert({at, cb});
}

absl::Status Engine::UpdateCode(const std::string& user,
                                const std::string& code) {
  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    code_updates_.push_back(CodeUpdate{user, code});
    loop_cv_.notify_all();
  }

  LOG(INFO) << "Code update queued";

  return absl::OkStatus();
}

}  // namespace midi
}  // namespace maethstro
