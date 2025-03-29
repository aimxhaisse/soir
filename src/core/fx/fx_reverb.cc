#include <absl/log/log.h>

#include "fx_reverb.hh"

namespace soir {
namespace fx {

Reverb::Reverb(Controls* controls)
    : controls_(controls),
      time_(0.01f, 0.0f, 1.0f),
      dry_(0.5f, 0.0f, 1.0f),
      wet_(0.5f, 0.0f, 1.0f),
      late_reverb_(dsp::LateReverb::MatrixFlavor::M16x16) {}

absl::Status Reverb::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  return absl::OkStatus();
}

bool Reverb::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void Reverb::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void Reverb::ReloadParams() {
  rapidjson::Document doc;

  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  time_ = Parameter::FromJSON(controls_, doc, "time");
  dry_ = Parameter::FromJSON(controls_, doc, "dry");
  wet_ = Parameter::FromJSON(controls_, doc, "wet");

  time_.SetRange(0.0f, 1.0f);
  dry_.SetRange(0.0f, 1.0f);
  wet_.SetRange(0.0f, 1.0f);
}

void Reverb::Render(SampleTick tick, AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    // Time is in seconds in the DSP code, we expand the indice to the
    // [0, 30s] range.
    auto time = time_.GetValue(current_tick) * 30.0f;

    early_params_.time_ = time;
    late_params_.time_ = time;

    early_reverb_.UpdateParameters(early_params_);
    late_reverb_.UpdateParameters(late_params_);

    auto p1 = early_reverb_.Process(lch[i], rch[i]);
    auto p2 = late_reverb_.Process(p1.first, p1.second);

    auto wet = wet_.GetValue(current_tick);
    auto dry = dry_.GetValue(current_tick);

    lch[i] = lch[i] * dry + p2.first * wet;
    rch[i] = rch[i] * dry + p2.second * wet;
  }
}

}  // namespace fx
}  // namespace soir
