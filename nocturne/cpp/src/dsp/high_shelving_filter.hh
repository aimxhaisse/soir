#pragma once

#include <tuple>

#include "dsp/dsp.hh"
#include "dsp/biquad_filter.hh"

namespace soir {
namespace dsp {

// A high shelving filter with human-friendly parameters.
class HighShelvingFilter {
 public:
  struct Parameters {
    float cutoff_ = 4000.0f;
    float boost_db_ = -20.0f;
  };

  HighShelvingFilter();

  void UpdateParameters(const Parameters& p);
  float Process(float input);

 private:
  void InitFromParameters();

  BiquadFilter::Parameters biquad_params_;
  BiquadFilter filter_;
  Parameters params_;
};

inline bool operator!=(const HighShelvingFilter::Parameters& lhs,
                       const HighShelvingFilter::Parameters& rhs) {
  return std::tie(lhs.cutoff_, lhs.boost_db_) !=
         std::tie(rhs.cutoff_, rhs.boost_db_);
}

}  // namespace dsp
}  // namespace soir