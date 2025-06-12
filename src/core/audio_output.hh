#pragma once

#include <SDL3/SDL.h>
#include <absl/status/status.h>
#include <memory>
#include <mutex>
#include <vector>

#include "core/dsp.hh"
#include "utils/config.hh"

namespace soir {

// This class consumes samples from the DSP engine and output them to
// the audio device. The first audio device is currently selected but
// we could later imagine configure it from code.
class AudioOutput : public SampleConsumer {
 public:
  AudioOutput();
  virtual ~AudioOutput();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

 private:
  static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

  SDL_AudioDeviceID device_id_ = 0;
  SDL_AudioStream* audio_stream_ = nullptr;
  std::vector<float> buffer_;
  std::mutex buffer_mutex_;
};

}  // namespace soir
