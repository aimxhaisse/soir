#pragma once

namespace soir {
namespace dsp {

float LeftPan(float pan) {
  return pan > 0.0f ? (1.0f - pan) : 1.0f;
}

float RightPan(float pan) {
  return pan < 0.0f ? (1.0f + pan) : 1.0f;
}

}  // namespace dsp
}  // namespace soir
