#pragma once

#include <condition_variable>
#include <mutex>

#include "common/config.hh"
#include "notifier.hh"

namespace maethstro {
namespace midi {

using CbFunc = std::function<void(const absl::Time& now)>;
using CbId = std::string;

// Scheduled callback at a given time.
struct ScheduledCb {
  absl::Time at;
  CbId id;

  bool operator()(const ScheduledCb& a, const ScheduledCb& b) const {
    return a.at < b.at;
  }
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

  void Schedule(const absl::Time& at, const CbId& id);
  void RegisterCb(const CbId& id, CbFunc func);
  void UnregisterCb(const CbId& id);

  // Those are part of the Live module and can be called from Python.
  void SetBPM(uint16_t bpm);
  uint16_t GetBPM() const;
  void Log(const std::string& user, const std::string& message);
  std::string GetUser() const;
  void Beat(const absl::Time& now);

 private:
  std::thread thread_;

  Notifier* notifier_;

  // Updated by the Python thread only.
  std::set<ScheduledCb, ScheduledCb> schedule_;
  std::map<CbId, CbFunc> callbacks_;

  // Updated by the main thread/gRPC threads.
  std::mutex loop_mutex_;
  std::condition_variable loop_cv_;
  std::list<CodeUpdate> code_updates_;
  bool running_ = false;

  uint64_t current_beat_ = 0;
  uint64_t beat_us_;
  uint16_t bpm_ = 120;
  std::string current_user_;
};

}  // namespace midi
}  // namespace maethstro
