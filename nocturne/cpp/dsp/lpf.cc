#include "dsp/lpf.hh"

namespace soir {
namespace dsp {

void LPF1P::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
  }
}

float LPF1P::Process(float xn) {
  float yn = (1.0 - params_.coefficient_) * xn + params_.coefficient_ * state_;

  state_ = yn;

  return yn;
}

}  // namespace dsp
}  // namespace soir
