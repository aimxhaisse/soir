#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "core/common.hh"

struct ma_device;

namespace soir {

class AudioBuffer;

namespace audio {

struct Device {
  int id;
  std::string name;
};

// Get available audio output devices
absl::StatusOr<std::vector<Device>> GetAudioOutDevices();

// Get available audio input devices
absl::StatusOr<std::vector<Device>> GetAudioInDevices();

class AudioOutput : public SampleConsumer {
 public:
  AudioOutput();
  ~AudioOutput();

  absl::Status Init(int sample_rate, int channels, int buffer_size);
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  // Buffer for storing pushed audio data (public for callback access)
  std::mutex buffer_mutex_;
  std::vector<float> audio_buffer_;

 private:
  ma_device* device_ = nullptr;
  bool initialized_ = false;
};

}  // namespace audio
}  // namespace soir
