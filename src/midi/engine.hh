#pragma once

#include <condition_variable>
#include <mutex>

#include "common/config.hh"
#include "notifier.hh"

namespace maethstro {
namespace midi {

using CbFunc = std::function<void()>;
using MicroBeat = uint64_t;

static constexpr uint64_t OneBeat = 1000000;

// Scheduled callback at a given beat.
struct Cb {
  // We use micro-beat units to prevent any precision loss with floats
  // which would lead to time drifts. We don't store any time here
  // because BPM can evolve at any moment, affecting the overall
  // scheduling.
  MicroBeat at;
  CbFunc func;

  bool operator()(const Cb& a, const Cb& b) const { return a.at < b.at; }
};

// Represents a code update coming from Matin.
struct CodeUpdate {
  std::string user;
  std::string code;
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

  absl::Status Init(const common::Config& config, Notifier* notifier);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

  // This is called from another thread to evaluate a piece of Python
  // code coming from Matin. Code is executed from the Run() loop.
  absl::Status UpdateCode(const std::string& user, const std::string& code);

  absl::Time MicroBeatToTime(MicroBeat beat) const;
  uint64_t MicroBeatToBeat(MicroBeat beat) const;
  void Schedule(MicroBeat at, const CbFunc& func);

  // Those are part of the Live module and can be called from Python.
  float SetBPM(float bpm);
  float GetBPM() const;
  void Log(const std::string& user, const std::string& message);
  std::string GetUser() const;
  void Beat();

 private:
  std::thread thread_;

  Notifier* notifier_;

  // Updated by the Python thread only.
  std::set<Cb, Cb> schedule_;

  // Updated by the main thread/gRPC threads.
  std::mutex loop_mutex_;
  std::condition_variable loop_cv_;
  std::list<CodeUpdate> code_updates_;
  bool running_ = false;

  MicroBeat current_beat_ = 0;
  absl::Time current_time_;
  float bpm_ = 120.0;
  uint64_t beat_us_ = 0;
  std::string current_user_;
};

}  // namespace midi
}  // namespace maethstro
