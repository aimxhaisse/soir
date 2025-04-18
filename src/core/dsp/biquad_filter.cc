#include "core/dsp/biquad_filter.hh"

namespace soir {
namespace dsp {

BiquadFilter::BiquadFilter() {}

void BiquadFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
  }
}

void BiquadFilter::Reset() {
  za1_ = 0.0;
  za2_ = 0.0;
  zb1_ = 0.0;
  zb2_ = 0.0;
}

float BiquadFilter::Process(float input) {
  const float yn = params_.a0_ * input + params_.a1_ * za1_ +
                   params_.a2_ * za2_ - params_.b1_ * zb1_ - params_.b2_ * zb2_;

  za2_ = za1_;
  za1_ = input;

  zb2_ = zb1_;
  zb1_ = yn;

  return yn;
}

}  // namespace dsp
}  // namespace soir