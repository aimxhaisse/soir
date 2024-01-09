#include "common.hh"

#include "audio_buffer.hh"

namespace maethstro {
namespace soir {

AudioBuffer::AudioBuffer(int num_samples) : size_(num_samples) {
  buffer_[kLeftChannel].resize(num_samples);
  buffer_[kRightChannel].resize(num_samples);
}

AudioBuffer::~AudioBuffer() {}

void AudioBuffer::ApplyGain(float gain) {
  for (int i = 0; i < kNumChannels; ++i) {
    for (int j = 0; j < size_; ++j) {
      buffer_[i][j] *= gain;
    }
  }
}

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
}  // namespace maethstro
