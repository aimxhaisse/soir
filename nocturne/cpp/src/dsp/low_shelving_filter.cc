#include "dsp/low_shelving_filter.hh"

#include <cmath>

namespace soir {
namespace dsp {

LowShelvingFilter::LowShelvingFilter() {
  InitFromParameters();
}

void LowShelvingFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

float LowShelvingFilter::Process(float input) {
  return filter_.Process(input);
}

void LowShelvingFilter::InitFromParameters() {
  const float theta_c = 2.0 * kPI * params_.cutoff_ / kSampleRate;
  const float mu = std::pow(10.0, -params_.boost_db_ / 20.0);
  const float beta = 4.0 / (1.0 + mu);
  const float delta = beta * std::tan(theta_c / 2.0);
  const float gamma = (1.0 - delta) / (1.0 + delta);

  biquad_params_.a0_ = (1.0 - gamma) / 2.0;
  biquad_params_.a1_ = (1.0 - gamma) / 2.0;
  biquad_params_.a2_ = 0.0;
  biquad_params_.b1_ = -gamma;
  biquad_params_.b2_ = 0.0;

  filter_.UpdateParameters(biquad_params_);
}

}  // namespace dsp
}  // namespace soir