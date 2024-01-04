#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "common.hh"
#include "common/config.hh"
#include "track.hh"

namespace maethstro {
namespace soir {

// First implementation is stupid and does not take
// into account lag. The timing precision of MIDI
// events is capped to a block size, we'll late see
// how to achieve intra-block precision
class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

  void RegisterConsumer(SampleConsumer* consumer);
  void RemoveConsumer(SampleConsumer* consumer);

 private:
  absl::Status Run();

  // Helper to print some statistics about CPU usage.
  void Stats(const absl::Time& next_block_at,
             const absl::Duration& block_duration) const;

  uint32_t block_size_;

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;

  std::list<SampleConsumer*> consumers_;
  std::list<std::unique_ptr<Track>> tracks_;
};

}  // namespace soir
}  // namespace maethstro
