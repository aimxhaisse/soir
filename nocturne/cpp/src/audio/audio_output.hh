#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "absl/status/status.h"
#include "core/common.hh"

struct ma_device;

namespace soir {

class AudioBuffer;

namespace audio {

class AudioOutput : public SampleConsumer {
 public:
  using AudioCallback = std::function<void(float* output, int frame_count)>;

  AudioOutput();
  ~AudioOutput();

  absl::Status Init(int sample_rate, int channels, int buffer_size);
  absl::Status Start();
  absl::Status Stop();
  void SetCallback(AudioCallback callback);

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  AudioCallback callback_;

  // Buffer for storing pushed audio data (public for callback access)
  std::mutex buffer_mutex_;
  std::vector<float> audio_buffer_;

 private:
  ma_device* device_ = nullptr;
  bool initialized_ = false;
};

}  // namespace audio
}  // namespace soir
