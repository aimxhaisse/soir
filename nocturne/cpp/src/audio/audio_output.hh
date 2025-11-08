#pragma once

#include "absl/status/status.h"

#include <functional>

struct ma_device;

namespace soir {
namespace audio {

class AudioOutput {
 public:
  using AudioCallback = std::function<void(float* output, int frame_count)>;

  AudioOutput();
  ~AudioOutput();

  absl::Status Init(int sample_rate, int channels, int buffer_size);
  absl::Status Start();
  absl::Status Stop();
  void SetCallback(AudioCallback callback);

  AudioCallback callback_;

 private:
  ma_device* device_ = nullptr;
  bool initialized_ = false;
};

}  // namespace audio
}  // namespace soir
