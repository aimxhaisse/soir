#include "modulated_delay.hh"

namespace soir {
namespace dsp {

ModulatedDelay::ModulatedDelay() {
  // Default construction, will be initialized with Init
}

void ModulatedDelay::Init(const Parameters& p) {
  params_ = p;
  InitFromParameters();
}

void ModulatedDelay::FastUpdate(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void ModulatedDelay::InitFromParameters() {
  lfo_params_.type_ = params_.type_;
  lfo_params_.frequency_ = params_.frequency_;

  lfo_.Init(lfo_params_);

  // We need to increase by one here because of float approximations.
  // At some point we'll want to allow automation of depth.
  delay_params_.max_ = 2 * params_.max_ + 1;
  delay_params_.size_ = params_.size_ + params_.depth_ + 1;

  delay_params_.interpolation_ = params_.interpolation_;

  delay_.Init(delay_params_);
}

void ModulatedDelay::SetModPhase(float phase) {
  lfo_.SetPhase(phase);
}

void ModulatedDelay::UpdateMod() {
  mod_ = lfo_.Render();
}

float ModulatedDelay::Read() {
  const float at = static_cast<float>(params_.size_) +
                   static_cast<float>(params_.depth_) * mod_;

  return delay_.ReadAt(at);
}

void ModulatedDelay::UpdateState(float xn) {
  delay_.Update(xn);
}

float ModulatedDelay::Render(float xn) {
  const float yn = Read();

  UpdateState(xn);
  UpdateMod();

  return yn;
}

void ModulatedDelay::Reset() {
  delay_.Reset();
  lfo_.Reset();
  mod_ = 0.0;
}

}  // namespace dsp
}  // namespace soir
