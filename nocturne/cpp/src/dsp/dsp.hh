#pragma once

#include <cmath>

namespace soir {

// Audio constants
static constexpr int kSampleRate = 48000;
static constexpr int kNumChannels = 2;
static constexpr int kLeftChannel = 0;
static constexpr int kRightChannel = 1;
static constexpr float kPI = 3.14159265358979323846f;

// Fast sine approximation for LFO use
inline float FastSin(float x) {
  // Simple sine approximation using Taylor series (good enough for LFO)
  return std::sin(x);
}

}  // namespace soir
