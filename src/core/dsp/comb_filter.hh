#pragma once

#include <tuple>
#include <vector>

#include "core/dsp/delay.hh"

namespace soir {
namespace dsp {

// DSP basic unit that may be useful for reverbs, this needs to be
// fast as it can be called N times per sample (usually N=4 or N=8).
//
// Intuitively, a feedback comb filter adds to the signal a delayed
// version of itself, creating interferences that have peaks in the
// output frequencies, making it look like a comb.
//
// Equation of the comb filter is:
//
// y[n] = x[n] + ay[n - K]
//
// Where:
//   - K is the size in samples (can be configured via size)
//   - a is the amount of gain of the delayed signal (feedback)
//
// As it only delays the signal by N sample only once, this unit is
// often used multiple times with different sizes, to provide a richer
// output signal. Using prime sizes ensures the delayed versions don't
// overlap.
class FeedbackCombFilter {
 public:
  struct Parameters {
    int max_ = 1;
    float size_ = 1.0f;
    float feedback_ = 0.5f;
  };

  FeedbackCombFilter();

  void Init(const Parameters& p);
  void UpdateParameters(const Parameters& p);
  void Reset();
  float Process(float input);

 private:
  // Initializes buffer from parameters.
  void InitFromParameters();

  Parameters params_;
  Delay::Parameters delayParams_;
  Delay delay_;
};

inline bool operator!=(const FeedbackCombFilter::Parameters& lhs,
                       const FeedbackCombFilter::Parameters& rhs) {
  return std::tie(lhs.max_, lhs.size_, lhs.feedback_) !=
         std::tie(rhs.max_, rhs.size_, rhs.feedback_);
};

}  // namespace dsp
}  // namespace soir
