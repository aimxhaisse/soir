#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

#include "absl/status/status.h"
#include "core/common.hh"

namespace soir {
namespace audio {

// PcmStream accumulates interleaved float32 stereo PCM samples into a ring
// buffer. It is registered as a SampleConsumer so the engine feeds it on every
// block. Python reads chunks via the pcm binding for low-latency WebSocket
// streaming to the browser.
class PcmStream : public SampleConsumer {
 public:
  PcmStream();
  ~PcmStream();

  absl::Status Init();
  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  size_t GetCurrentOffset() const;

  struct ReadResult {
    std::vector<float> data;
    size_t new_offset;
  };

  // Blocks until the engine produces a new block or 50ms elapses (for
  // graceful shutdown). Returns interleaved float32 samples since offset,
  // capped to one block.
  ReadResult Read(size_t offset) const;

 private:
  bool initialized_ = false;

  mutable std::mutex buffer_mutex_;
  mutable std::condition_variable buffer_cv_;

  // Ring buffer of interleaved float32 samples (L, R, L, R, ...).
  std::vector<float> ring_buffer_;
  size_t ring_capacity_ = 0;
  size_t write_pos_ = 0;
};

}  // namespace audio
}  // namespace soir
