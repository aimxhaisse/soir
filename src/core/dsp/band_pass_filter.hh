#pragma once

#include <tuple>

#include "core/dsp.hh"
#include "core/dsp/biquad_filter.hh"

namespace soir {
namespace dsp {

// A band-pass filter.
class BandPassFilter {
 public:
  struct Parameters {
    float frequency_ = 3000.0f;
    float width_coefficient_ = 0.8f;
    float boost_db_ = -20.0f;
  };

  // Maybe provide a way to configure this. This defines the range of
  // tuning we provide in the frequency response. Min value defines
  // how peaky the curve can be, max value how flat it can be.
  static constexpr float kMinActualWidth = 0.01;
  static constexpr float kMaxActualWidth = 10.0;

  BandPassFilter();

  void UpdateParameters(const Parameters& p);
  void Reset();
  float Process(float input);

 private:
  void InitFromParameters();

  Parameters params_;
  float gain_ = 1.0f;
  BiquadFilter::Parameters biquad_params_;
  BiquadFilter filter_;
};

inline bool operator!=(const BandPassFilter::Parameters& lhs,
                       const BandPassFilter::Parameters& rhs) {
  return std::tie(lhs.frequency_, lhs.width_coefficient_,
                  lhs.boost_db_) != 
         std::tie(rhs.frequency_, rhs.width_coefficient_,
                  rhs.boost_db_);
}

}  // namespace dsp
}  // namespace soir