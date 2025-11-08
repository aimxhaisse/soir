#pragma once

#include <cstdint>

namespace soir {
namespace dsp {

// A super-fast random generator with a low entropy. Fine for DSP
// algorithms that need a bit of surprises.
class FastRandom {
 public:
  void Seed(uint32_t seed);

  uint32_t URandom();
  uint32_t UBetween(uint32_t min, uint32_t max);
  float FBetween(float min, float max);

 private:
  uint32_t seed_ = 0x1240FE03;
};

}  // namespace dsp
}  // namespace soir
