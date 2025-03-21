#include "tools.hh"

#include <cmath>

namespace soir {


float LeftPan(float pan) {
  return pan > 0.0f ? (1.0f - pan) : 1.0f;
}

float RightPan(float pan) {
  return pan < 0.0f ? (1.0f + pan) : 1.0f;
}

float Bipolar(float unipolar) {
  return (unipolar - 0.5f) * 2.0f;
}

float Unipolar(float bipolar) {
  return (bipolar + 1.0f) / 2.0f;
}

float Fabs(float value) {
  return value < 0.0f ? -value : value;
}

float FastSin(float x) {
  // Investigate alternatives once we have real usecases where we need
  // to go fast. No need to waste time on this for now.
  return sin(x);
}


}  // namespace soir
