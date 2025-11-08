#include "dsp/low_pass_filter.hh"

#include <algorithm>
#include <cmath>

namespace soir {
namespace dsp {

LowPassFilter::LowPassFilter() {
  InitFromParameters();
}

void LowPassFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void LowPassFilter::Reset() {
  filter_.Reset();
}

float LowPassFilter::Process(float input) {
  return filter_.Process(input);
}

void LowPassFilter::InitFromParameters() {
  // Clamp parameters to reasonable ranges
  const float cutoff = std::max(20.0f, std::min(params_.cutoff_, 20000.0f));
  const float res = std::max(0.0f, std::min(params_.resonance_, 1.0f));

  // Normalized cutoff frequency (0 to pi)
  const float w0 = 2.0f * kPI * cutoff / kSampleRate;
  const float cos_w0 = std::cos(w0);
  const float sin_w0 = std::sin(w0);

  // Convert resonance to Q factor (0.5 to ~25)
  // As resonance increases from 0 to 1, Q increases exponentially
  const float q = 0.5f + 24.5f * std::pow(res, 2.0f);

  // Calculate alpha term
  const float alpha = sin_w0 / (2.0f * q);

  // Low-pass filter coefficient calculation
  const float a0 = 1.0f + alpha;

  biquad_params_.a0_ = (1.0f - cos_w0) / (2.0f * a0);
  biquad_params_.a1_ = (1.0f - cos_w0) / a0;
  biquad_params_.a2_ = (1.0f - cos_w0) / (2.0f * a0);
  biquad_params_.b1_ = (-2.0f * cos_w0) / a0;
  biquad_params_.b2_ = (1.0f - alpha) / a0;

  filter_.UpdateParameters(biquad_params_);
}

}  // namespace dsp
}  // namespace soir