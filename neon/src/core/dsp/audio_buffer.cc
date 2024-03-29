#include "utils/misc.hh"

#include "core/dsp/audio_buffer.hh"
#include "core/dsp/dsp.hh"

namespace neon {
namespace dsp {

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

void AudioBuffer::ApplyPan(float pan) {
  // Right Pan
  if (pan > 0.5f) {
    const float gain = std::abs(pan - 1.0f) * 2.0f;
    for (int i = 0; i < size_; ++i) {
      buffer_[kLeftChannel][i] *= gain;
    }
    return;
  }

  // Left pan
  if (pan < 0.5f) {
    const float gain = pan * 2.0f;
    for (int i = 0; i < size_; ++i) {
      buffer_[kRightChannel][i] *= gain;
    }
    return;
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

}  // namespace dsp
}  // namespace neon
