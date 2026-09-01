#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
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

// Get available audio output devices
absl::StatusOr<std::vector<Device>> GetAudioOutDevices();

// Get available audio input devices
absl::StatusOr<std::vector<Device>> GetAudioInDevices();

class AudioOutput : public SampleConsumer {
 public:
  AudioOutput();
  ~AudioOutput();

  absl::Status Init(int sample_rate, int channels, int buffer_size,
                    const std::string& device_name = "");
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  // Number of device callbacks that had to insert silence because the
  // engine had not pushed audio in time (underruns). A non-zero count
  // while listening is the signature of audible crackles.
  uint64_t GetUnderrunCount() const {
    return underruns_.load(std::memory_order_relaxed);
  }

  // Called from the device callback when silence was inserted. Counts
  // the underrun and logs a rate-limited warning (device thread; the
  // log lock is held for a single line at most once per second, which
  // the device's own buffer absorbs). Public because the miniaudio
  // data callback is a free function.
  void OnUnderrun(size_t missing_samples);

  // Buffer for storing pushed audio data (public for callback access)
  std::mutex buffer_mutex_;
  std::vector<float> audio_buffer_;

 private:
  ma_context context_;
  bool context_initialized_ = false;
  ma_device* device_ = nullptr;
  ma_device_id selected_device_id_;
  bool initialized_ = false;

  std::atomic<uint64_t> underruns_{0};
  // Device-thread-only, used to rate-limit the underrun warning.
  absl::Time last_underrun_warn_{};
};

}  // namespace audio
}  // namespace soir
