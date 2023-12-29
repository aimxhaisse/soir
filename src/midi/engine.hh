#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <mutex>

#include "common/config.hh"

namespace py = pybind11;

namespace maethstro {

struct Callback {
  std::chrono::high_resolution_clock::time_point time;
  std::function<absl::Status()> func;
};

class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const Config& config);
  absl::Status Run();
  absl::Status Stop();
  absl::Status Wait();

  // This is called from another thread to evaluate a piece of Python
  // code coming from Matin.
  absl::Status UpdateCode(const std::string& code);

  absl::Status PushCallback(std::function<absl::Status()> func);

  absl::Status OnBeat();

 private:
  std::list<Callback> callbacks_;

  std::mutex loop_mutex_;
  std::condition_variable loop_cv_;
  std::list<std::string> code_updates_;
  bool running_;
};

}  // namespace maethstro
