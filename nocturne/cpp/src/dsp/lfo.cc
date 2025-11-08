#include "dsp/lfo.hh"
#include "dsp/tools.hh"
#include "dsp/dsp.hh"

#include <algorithm>
#include <cmath>

namespace soir {
namespace dsp {

LFO::LFO() {
  // Default construction, will be initialized with Init
}

void LFO::Init(const Parameters& p) {
  if (p != params_) {
    params_ = p;
    InitFromParameters();
  }
}

void LFO::SetPhase(float phase) {
  value_ = phase;
  last_phase_ = phase;
}

void LFO::Reset() {
  value_ = last_phase_;
}

void LFO::InitFromParameters() {
  inc_ = params_.frequency_ / kSampleRate;
}

float LFO::Render() {
  value_ += inc_;
  if (value_ >= 1.0f) {
    value_ -= 1.0f;
  }

  const float v = Bipolar(value_);
  float result = 0.0f;

  switch (params_.type_) {
    case SAW:
      result = v;
      break;

    case TRI:
      result = 2.0f * std::abs(v) - 1.0f;
      break;

    case SINE:
      result = soir::FastSin(v * kPI);
      break;
  };

  result = std::max(-1.0f, std::min(1.0f, result));

  return result;
}

}  // namespace dsp
}  // namespace soir
