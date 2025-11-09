#pragma once

#include <tuple>

#include "core/common.hh"
#include "dsp/biquad_filter.hh"

namespace soir {
namespace dsp {

// A high-pass filter with controllable cutoff frequency and resonance.
class HighPassFilter {
 public:
  struct Parameters {
    float cutoff_ = 1000.0f;  // Cutoff frequency in Hz (20-20000Hz)
    float resonance_ = 0.5f;  // Resonance coefficient (0.0-1.0)
  };

  HighPassFilter();

  void UpdateParameters(const Parameters& p);
  void Reset();
  float Process(float input);

 private:
  void InitFromParameters();

  Parameters params_;
  BiquadFilter::Parameters biquad_params_;
  BiquadFilter filter_;
};

inline bool operator!=(const HighPassFilter::Parameters& lhs,
                       const HighPassFilter::Parameters& rhs) {
  return std::tie(lhs.cutoff_, lhs.resonance_) !=
         std::tie(rhs.cutoff_, rhs.resonance_);
}

}  // namespace dsp
}  // namespace soir