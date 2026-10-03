#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "core/common.hh"
#include "miniaudio.h"

namespace soir {

class AudioBuffer;

namespace audio {

struct Device {
  int id;
  std::string name;
  bool is_default;
  int channels;
};

absl::StatusOr<std::vector<Device>> GetAudioOutDevices();
absl::StatusOr<std::vector<Device>> GetAudioInDevices();

// One slot holds one engine block; kOutputSlotCount is sized so the
// backlog cap never wraps onto the slot the device thread is reading.
static constexpr int kOutputSlotFrames = kBlockSize;
static constexpr int kOutputSlotSamples = kOutputSlotFrames * kNumChannels;
static constexpr int kOutputSlotCount = 64;
static constexpr int kOutputMaxBacklogSlots = 47;

// Lock-free ring of interleaved blocks between the engine thread (sole
// producer) and the miniaudio device callback (sole consumer). The
// device buffer plus a prefill of silence absorb engine stalls, so the
// two threads never block each other.
class AudioOutput : public SampleConsumer {
 public:
  AudioOutput();
  ~AudioOutput() override;

  absl::Status Init(int sample_rate, int channels, int buffer_size,
                    const std::string& device_name = "");
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  // Device callbacks that inserted silence because the engine was late.
  uint64_t GetUnderrunCount() const {
    return underruns_.load(std::memory_order_relaxed);
  }

  // Counts an underrun and logs a rate-limited warning.
  void OnUnderrun(size_t missing_samples);

  // Device thread: copy from the ring into output, padding the tail with
  // silence on underrun. Public because the miniaudio data callback is a
  // free function.
  void FillCallbackBuffer(float* output, size_t samples_needed);

  // Engine thread: clear the ring. Safe only while the device callback
  // is not running.
  void Reset();

 private:
  struct Slot {
    float samples[kOutputSlotSamples];
  };

  void WriteSlot(const float* left, const float* right, size_t frames);

  ma_context context_;
  bool context_initialized_;
  ma_device* device_;
  ma_device_id selected_device_id_;
  bool initialized_;

  std::atomic<uint64_t> underruns_;
  // Device-thread-only.
  absl::Time last_underrun_warn_;

  Slot slots_[kOutputSlotCount];

  // Single-writer each: write_seq_/drop_seq_ by the engine, read_seq_ by
  // the device; drop_seq_ is the floor the device resyncs to after drops.
  std::atomic<uint64_t> write_seq_;
  std::atomic<uint64_t> read_seq_;
  std::atomic<uint64_t> drop_seq_;

  uint64_t write_seq_local_;
  uint64_t drop_seq_local_;
  uint64_t read_seq_local_;
  int read_offset_;

  // Pre-fill silence (by the sole producer) and mark "engine is pushing":
  // underruns before the first push are a startup artifact, not counted.
  std::atomic<bool> prefilled_;
};

}  // namespace audio
}  // namespace soir
