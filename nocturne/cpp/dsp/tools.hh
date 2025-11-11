#pragma once

#include <cmath>

namespace soir {
namespace dsp {

float Unipolar(float bipolar);
float Bipolar(float unipolar);

// Fast sine approximation for LFO use
inline float FastSin(float x) {
  // Simple sine approximation using Taylor series (good enough for LFO)
  return std::sin(x);
}

}  // namespace dsp
}  // namespace soir
