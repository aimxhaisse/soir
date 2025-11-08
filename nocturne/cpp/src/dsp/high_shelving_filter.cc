#include "dsp/high_shelving_filter.hh"

#include <cmath>

namespace soir {
namespace dsp {

HighShelvingFilter::HighShelvingFilter() {
  InitFromParameters();
}

void HighShelvingFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

float HighShelvingFilter::Process(float input) {
  return filter_.Process(input);
}

void HighShelvingFilter::InitFromParameters() {
  const float theta_c = 2.0 * kPI * params_.cutoff_ / kSampleRate;
  const float mu = std::pow(10.0, -params_.boost_db_ / 20.0);
  const float beta = (1.0 + mu) / 4.0;
  const float delta = beta * std::tan(theta_c / 2.0);
  const float gamma = (1.0 - delta) / (1.0 + delta);

  biquad_params_.a0_ = (1.0 + gamma) / 2.0;
  biquad_params_.a1_ = -biquad_params_.a0_;
  biquad_params_.a2_ = 0.0;
  biquad_params_.b1_ = -gamma;
  biquad_params_.b2_ = 0.0;

  filter_.UpdateParameters(biquad_params_);
}

}  // namespace dsp
}  // namespace soir