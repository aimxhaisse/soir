#include "utils/misc.hh"

#include "core/dsp/audio_buffer.hh"
#include "core/dsp/dsp.hh"

namespace soir {
namespace dsp {

AudioBuffer::AudioBuffer(int num_samples) : size_(num_samples) {
  buffer_[kLeftChannel].resize(num_samples);
  buffer_[kRightChannel].resize(num_samples);
}

AudioBuffer::~AudioBuffer() {}

std::size_t AudioBuffer::Size() const {
  return size_;
}

float* AudioBuffer::GetChannel(int channel) {
  return buffer_[channel].data();
}

void AudioBuffer::Reset() {
  for (int i = 0; i < kNumChannels; ++i) {
    std::fill(buffer_[i].begin(), buffer_[i].end(), 0.0f);
  }
}

}  // namespace dsp
}  // namespace soir
