#include "fx_reverb.hh"

#include <absl/log/log.h>

namespace soir {
namespace fx {

Reverb::Reverb(Controls* controls)
    : controls_(controls),
      time_(0.01f, 0.0f, 1.0f),
      dry_(0.5f, 0.0f, 1.0f),
      wet_(0.5f, 0.0f, 1.0f) {}

absl::Status Reverb::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  reverb_.Reset();

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

    auto time = time_.GetValue(current_tick);

    params_.time_ = time;

    if (!initialized_) {
      reverb_.Init(params_);
      initialized_ = true;
    } else {
      reverb_.UpdateParameters(params_);
    }

    auto p = reverb_.Process(lch[i], rch[i]);

    auto wet = wet_.GetValue(current_tick);
    auto dry = dry_.GetValue(current_tick);

    lch[i] = lch[i] * dry + p.first * wet;
    rch[i] = rch[i] * dry + p.second * wet;
  }
}

}  // namespace fx
}  // namespace soir
