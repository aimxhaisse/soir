#include "chorus.hh"

namespace soir {
namespace dsp {

Chorus::Chorus() {
  // Default construction, will be initialized with Init
}

void Chorus::Init(const Parameters& p) {
  params_ = p;
  InitFromParameters();

  left_.SetModPhase(0.25);
  center_.SetModPhase(0.0);
  right_.SetModPhase(0.75);
}

void Chorus::FastUpdate(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void Chorus::Reset() {
  left_.Reset();
  center_.Reset();
  right_.Reset();
}

void Chorus::InitFromParameters() {
  left_params_.type_ = LFO::TRI;
  center_params_.type_ = LFO::TRI;
  right_params_.type_ = LFO::TRI;

  // This is borrowed from Pirkle's note about chorus effects. It
  // sounds generally good across the range.
  static constexpr int kMinDelay = 100;
  static constexpr int kMaxDelay = 500;
  static constexpr int kDepth = 90;

  const int max = static_cast<int>(kMaxDelay + kDepth) + 1;
  const float depth = params_.depth_ * kDepth;
  const float size = (kMinDelay + params_.time_ * (kMaxDelay - kMinDelay));

  left_params_.max_ = max;
  center_params_.max_ = max;
  right_params_.max_ = max;

  left_params_.size_ = size;
  center_params_.size_ = size;
  right_params_.size_ = size;

  left_params_.depth_ = depth;
  center_params_.depth_ = depth;
  right_params_.depth_ = depth;

  left_params_.frequency_ = params_.rate_;
  center_params_.frequency_ = params_.rate_;
  right_params_.frequency_ = params_.rate_;

  left_.Init(left_params_);
  center_.Init(center_params_);
  right_.Init(right_params_);
}

std::pair<float, float> Chorus::Render(float lxn, float rxn) {
  const float lyn = left_.Render(lxn);
  const float cyn = center_.Render((lxn + rxn) / 2.0f);
  const float ryn = right_.Render(rxn);

  return {lyn + cyn, ryn + cyn};
}

}  // namespace dsp
}  // namespace soir
