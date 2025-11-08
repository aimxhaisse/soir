#include "dsp/band_pass_filter.hh"

#include <cmath>

namespace soir {
namespace dsp {

BandPassFilter::BandPassFilter() {
  InitFromParameters();
}

void BandPassFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void BandPassFilter::Reset() {
  filter_.Reset();
}

float BandPassFilter::Process(float input) {
  return gain_ * filter_.Process(input);
}

void BandPassFilter::InitFromParameters() {
  const float k = std::tan((kPI * params_.frequency_) / kSampleRate);
  const float k2 = k * k;
  const float bw = kMinActualWidth + (kMaxActualWidth - kMinActualWidth) *
                                         params_.width_coefficient_;
  const float q = 1.0f / bw;
  const float delta = k2 * q + k + q;

  biquad_params_.a0_ = k / delta;
  biquad_params_.a1_ = 0.0f;
  biquad_params_.a2_ = -biquad_params_.a0_;
  biquad_params_.b1_ = (2.0f * q * (k2 - 1.0f)) / delta;
  biquad_params_.b2_ = (k2 * q - k + q) / delta;

  gain_ = std::pow(10.0, params_.boost_db_ / 20.0);

  filter_.UpdateParameters(biquad_params_);
}

}  // namespace dsp
}  // namespace soir