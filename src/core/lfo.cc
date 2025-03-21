#include "core/lfo.hh"

#include "core/dsp.hh"
#include "core/tools.hh"

namespace soir {


LFO::LFO() {}

absl::Status LFO::Init(const Settings& s) {
  settings_ = s;
  inc_ = settings_.frequency_ / kSampleRate;

  return absl::OkStatus();
}

void LFO::setPhase(float coef) {
  value_ = coef;
  lastPhase_ = coef;
}

float LFO::Render() {
  value_ += inc_;
  if (value_ >= 1.0f) {
    value_ -= 1.0f;
  }

  const float v = Bipolar(value_);
  float result = 0.0f;

  switch (settings_.type_) {
    case SAW:
      result = v;
      break;

    case TRI:
      result = 2.0f * Fabs(v) - 1.0f;
      break;

    case SINE:
      result = FastSin(v * kPi);
      break;
  };

  result = std::max(-1.0f, std::min(1.0f, result));

  return result;
}


}  // namespace soir
