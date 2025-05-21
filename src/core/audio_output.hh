#pragma once

#include <SDL2/SDL.h>
#include <absl/status/status.h>

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

  // Get the list of available audio output devices (device number and associated name).
  static absl::Status GetAudioDevices(
      std::vector<std::pair<int, std::string>>* out);

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

 private:
  SDL_AudioDeviceID device_id_ = 0;
};

}  // namespace soir
