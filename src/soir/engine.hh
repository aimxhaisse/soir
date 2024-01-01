#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common/config.hh"

namespace maethstro {
namespace soir {

class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  absl::Status Run();

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;
};

}  // namespace soir
}  // namespace maethstro
