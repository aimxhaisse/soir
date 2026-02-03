#include "dsp/delayed_apf.hh"

namespace soir {
namespace dsp {

DelayedAPF::DelayedAPF() { InitFromParameters(); }

void DelayedAPF::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void DelayedAPF::InitFromParameters() {
  delayParams_.max_ = params_.max_;
  delayParams_.size_ = params_.size_;
  delayParams_.interpolation_ = params_.interpolation_;
  delayParams_.depth_ = params_.depth_;
  delayParams_.frequency_ = params_.frequency_;
  delayParams_.type_ = params_.type_;

  delay_.Init(delayParams_);

  lpfParams_.coefficient_ = params_.lpf_;
  lpf_.UpdateParameters(lpfParams_);
}

float DelayedAPF::Process(float xn) {
  const float zd = delay_.Read();
  const float gn = params_.coef_ * zd;
  const float wn = lpf_.Process(gn + xn * params_.mix_);
  const float yn = -params_.coef_ * wn + zd;

  delay_.UpdateState(wn);
  delay_.UpdateMod();

  return yn;
}

void DelayedAPF::Reset() { delay_.Reset(); }

void DelayedAPF::SetModPhase(float coef) { delay_.SetModPhase(coef); }

}  // namespace dsp
}  // namespace soir
