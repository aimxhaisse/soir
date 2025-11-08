#include "dsp/comb_filter.hh"

namespace soir {
namespace dsp {

FeedbackCombFilter::FeedbackCombFilter() {
  InitFromParameters();
}

void FeedbackCombFilter::InitFromParameters() {
  delayParams_.max_ = params_.max_;
  delayParams_.size_ = params_.size_;

  delay_.Init(delayParams_);
}

void FeedbackCombFilter::Init(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void FeedbackCombFilter::UpdateParameters(const Parameters& p) {
  Init(p);
}

void FeedbackCombFilter::Reset() {
  delay_.Reset();
}

float FeedbackCombFilter::Process(float input) {
  const float yn = delay_.ReadAt(params_.size_) * params_.feedback_ + input;

  delay_.Update(yn);

  return yn;
}

}  // namespace dsp
}  // namespace soir
