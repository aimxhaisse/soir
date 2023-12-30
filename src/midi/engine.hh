#pragma once

#include <condition_variable>
#include <mutex>

#include "common/config.hh"
#include "notifier.hh"

namespace maethstro {

using CallbackFunc = std::function<absl::Status(const absl::Time& now)>;

struct Callback {
  // When the callback is meant to be executed.
  absl::Time at;

  // Callbacks take the current time to be able to perform
  // temporal recursion without drifts.
  CallbackFunc func;

  bool operator()(const Callback& a, const Callback& b) const {
    return a.at < b.at;
  }
};

// This is the main engine that runs the Python code and schedules
// callbacks. It uses a temporal recursion pattern to avoid time
// drifts (this is heavily inspired from Extempore & Sonic PI).
//
// Threading model:
//
// - all Python is executed from the running loop
// - all callbacks are executed from the running loop
// - code updates are pushed from an external thread via UpdateCode()
//
// This allows callbacks to interact with Python without GIL issues.
//
// We'll need to update this model in case we want to schedule
// callbacks from other threads.
class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const Config& config, Notifier* notifier);
  absl::Status Run();
  absl::Status Stop();
  absl::Status Wait();

  // This is called from another thread to evaluate a piece of Python
  // code coming from Matin. Code is executed from the Run() loop.
  absl::Status UpdateCode(const std::string& code);
  absl::Status Schedule(const absl::Time& at, CallbackFunc func);
  absl::Status Beat(const absl::Time& now);

  // Those are part of the Live module and can be called from Python.
  void Live_SetBPM(uint16_t bpm);
  uint16_t Live_GetBPM() const;
  void Live_Log(const std::string& message);

 private:
  Notifier* notifier_;

  // Updated by the Python thread only.
  std::set<Callback, Callback> callbacks_;

  // Updated by the main thread/gRPC threads.
  std::mutex loop_mutex_;
  std::condition_variable loop_cv_;
  std::list<std::string> code_updates_;
  bool running_ = false;

  uint64_t current_beat_ = 0;
  uint64_t beat_us_;
  uint16_t bpm_ = 120;
};

}  // namespace maethstro
