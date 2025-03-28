#include "utils/misc.hh"

#include "core/audio_buffer.hh"
#include "core/dsp.hh"

namespace soir {

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

}  // namespace soir
