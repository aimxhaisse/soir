#pragma once

#include <vector>

namespace maethstro {
namespace soir {

class AudioBuffer {
 public:
  explicit AudioBuffer(int num_samples);
  ~AudioBuffer();

  AudioBuffer(const AudioBuffer&) = default;
  AudioBuffer& operator=(const AudioBuffer&) = default;

  void ApplyGain(float gain);
  void ApplyPan(float pan);
  std::size_t Size() const;
  float* GetChannel(int channel);
  void Reset();

 private:
  std::size_t size_;
  std::vector<float> buffer_[2];
};

}  // namespace soir
}  // namespace maethstro
