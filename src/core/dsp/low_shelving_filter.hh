#pragma once

#include <tuple>

#include "core/dsp.hh"
#include "core/dsp/biquad_filter.hh"

namespace soir {
namespace dsp {

// A low shelving filter with human-friendly parameters.
class LowShelvingFilter {
 public:
  struct Parameters {
    float cutoff_ = 150.0f;
    float boost_db_ = -20.0f;
  };

  LowShelvingFilter();

  void UpdateParameters(const Parameters& p);
  float Process(float input);

 private:
  void InitFromParameters();

  BiquadFilter::Parameters biquad_params_;
  BiquadFilter filter_;
  Parameters params_;
};

inline bool operator!=(const LowShelvingFilter::Parameters& lhs,
                       const LowShelvingFilter::Parameters& rhs) {
  return std::tie(lhs.cutoff_, lhs.boost_db_) !=
         std::tie(rhs.cutoff_, rhs.boost_db_);
}

}  // namespace dsp
}  // namespace soir