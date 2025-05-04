#include "fast_random.hh"

namespace soir {
namespace dsp {

void FastRandom::Seed(uint32_t seed) {
  seed_ = seed;
}

uint32_t FastRandom::URandom() {
  // Lehmer RNG, formula stolen from Wikipedia.
  seed_ = static_cast<uint32_t>(static_cast<uint64_t>(seed_) * 279470273u %
                                0xFFFFFFFb);

  return seed_;
}

uint32_t FastRandom::UBetween(uint32_t min, uint32_t max) {
  return URandom() % (max - min) + min;
}

float FastRandom::FBetween(float min, float max) {
  const float scaled =
      static_cast<float>(URandom()) / static_cast<float>(UINT32_MAX);

  return scaled * (max - min) + min;
}

}  // namespace dsp
}  // namespace soir
