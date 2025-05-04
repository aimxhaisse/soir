#include "core/dsp/two_band_shelving_filter.hh"

namespace soir {
namespace dsp {

TwoBandShelvingFilter::TwoBandShelvingFilter() {
  InitFromParameters();
}

void TwoBandShelvingFilter::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

float TwoBandShelvingFilter::Process(float input) {
  return filter_low_.Process(filter_high_.Process(input));
}

void TwoBandShelvingFilter::InitFromParameters() {
  filter_low_.UpdateParameters(params_.low_params_);
  filter_high_.UpdateParameters(params_.high_params_);
}

}  // namespace dsp
}  // namespace soir