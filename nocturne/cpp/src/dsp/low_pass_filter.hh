#pragma once

#include <tuple>

#include "dsp/biquad_filter.hh"
#include "dsp/dsp.hh"

namespace soir {
namespace dsp {

// A low-pass filter with controllable cutoff frequency and resonance.
class LowPassFilter {
 public:
  struct Parameters {
    float cutoff_ = 2000.0f;  // Cutoff frequency in Hz (20-20000Hz)
    float resonance_ = 0.5f;  // Resonance coefficient (0.0-1.0)
  };

  LowPassFilter();

  void UpdateParameters(const Parameters& p);
  void Reset();
  float Process(float input);

 private:
  void InitFromParameters();

  Parameters params_;
  BiquadFilter::Parameters biquad_params_;
  BiquadFilter filter_;
};

inline bool operator!=(const LowPassFilter::Parameters& lhs,
                       const LowPassFilter::Parameters& rhs) {
  return std::tie(lhs.cutoff_, lhs.resonance_) !=
         std::tie(rhs.cutoff_, rhs.resonance_);
}

}  // namespace dsp
}  // namespace soir