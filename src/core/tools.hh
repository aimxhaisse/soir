#pragma once

namespace soir {

static constexpr float kPi = 3.14159265358979323846f;

float LeftPan(const float pan);
float RightPan(const float pan);
float Bipolar(const float value);
float Unipolar(const float value);
float Fabs(const float value);
float FastSin(const float x);
float Clip(const float value, const float min, const float max);

}  // namespace soir
