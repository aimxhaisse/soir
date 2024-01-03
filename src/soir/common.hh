#pragma once

#include <absl/status/status.h>
#include <vector>

#include "audio_buffer.hh"

namespace maethstro {
namespace soir {

static constexpr int kNumChannels = 2;
static constexpr int kLeftChannel = 0;
static constexpr int kRightChannel = 1;
static constexpr int kSampleRate = 48000;
static constexpr float kVorbisQuality = 1.0f;

class SampleConsumer {
 public:
  virtual absl::Status PushAudioBuffer(const AudioBuffer&) = 0;
};

}  // namespace soir
}  // namespace maethstro
