#pragma once

namespace soir {
namespace dsp {

static constexpr float kPi = 3.14159265358979323846f;

float LeftPan(float pan);
float RightPan(float pan);
float Bipolar(float value);
float Unipolar(float value);
float Fabs(float value);
float FastSin(float x);

}  // namespace dsp
}  // namespace soir
