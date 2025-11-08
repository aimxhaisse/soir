#pragma once

#include <cmath>

namespace soir {

// Audio constants needed by DSP components
static constexpr int kSampleRate = 48000;
static constexpr float kPI = 3.14159265358979323846f;

// Fast sine approximation for LFO use
inline float FastSin(float x) {
  // Simple sine approximation using Taylor series (good enough for LFO)
  return std::sin(x);
}

}  // namespace soir
