#pragma once

#include <tuple>

namespace soir {
namespace dsp {

// Base for all of our filters, the difference is how we parameterize
// them. This is a rather low-level class, better use its subclasses
// which have more human-friendly parameters (frequency cutoff, etc).
class BiquadFilter {
 public:
  struct Parameters {
    float a0_ = 0.0;
    float a1_ = 0.0;
    float a2_ = 0.0;
    float b1_ = 0.0;
    float b2_ = 0.0;
  };

  BiquadFilter();

  void UpdateParameters(const Parameters& p);
  void Reset();

  float Process(float input);

 protected:
  Parameters params_;

 private:
  float za1_ = 0.0;
  float za2_ = 0.0;
  float zb1_ = 0.0;
  float zb2_ = 0.0;
};

inline bool operator!=(const BiquadFilter::Parameters& lhs,
                       const BiquadFilter::Parameters& rhs) {
  return std::tie(lhs.a0_, lhs.a1_, lhs.a2_, lhs.b1_, lhs.b2_) !=
         std::tie(rhs.a0_, rhs.a1_, rhs.a2_, rhs.b1_, rhs.b2_);
}

}  // namespace dsp
}  // namespace soir