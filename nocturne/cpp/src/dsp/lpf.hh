#pragma once

#include <tuple>

namespace soir {
namespace dsp {

// A single pole LPF for which the coefficient G can be edited. The
// expected range is [0.0, 1.0]. The higher the value, the lower the
// cutoff point in the frequency response.
//
// Corresponding difference equation:
//
// y[n] = (1.0 - g) * x[n] + g * y[n - 1]
class LPF1P {
 public:
  struct Parameters {
    float coefficient_ = 1.0;
  };

  void UpdateParameters(const Parameters& p);

  float Process(float xn);

 private:
  Parameters params_;
  float state_ = 0.0f;
};

inline bool operator!=(const LPF1P::Parameters& lhs,
                       const LPF1P::Parameters& rhs) {
  return std::tie(lhs.coefficient_) != std::tie(rhs.coefficient_);
};

}  // namespace dsp
}  // namespace soir
