#pragma once

#include "core/dsp/high_shelving_filter.hh"
#include "core/dsp/low_shelving_filter.hh"

namespace soir {
namespace dsp {

// A two-band shelving filter with human-friendly parameters.
class TwoBandShelvingFilter {
 public:
  struct Parameters {
    HighShelvingFilter::Parameters high_params_;
    LowShelvingFilter::Parameters low_params_;
  };

  TwoBandShelvingFilter();

  void UpdateParameters(const Parameters& p);
  float Process(float input);

 private:
  void InitFromParameters();

  LowShelvingFilter filter_low_;
  HighShelvingFilter filter_high_;
  Parameters params_;
};

inline bool operator!=(const TwoBandShelvingFilter::Parameters& lhs,
                       const TwoBandShelvingFilter::Parameters& rhs) {
  return (lhs.high_params_ != rhs.high_params_) ||
         (lhs.low_params_ != rhs.low_params_);
}

}  // namespace dsp
}  // namespace soir