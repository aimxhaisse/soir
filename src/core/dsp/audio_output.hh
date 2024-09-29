#pragma once

#include <SDL2/SDL.h>
#include <absl/status/status.h>

#include "core/dsp/dsp.hh"
#include "utils/config.hh"

namespace neon {
namespace dsp {

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
  SDL_AudioDeviceID device_id_ = 0;
};

}  // namespace dsp
}  // namespace neon
