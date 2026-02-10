#include "fx_chorus.hh"

#include <absl/log/log.h>

#include "utils/tools.hh"

namespace soir {
namespace fx {

Chorus::Chorus(Controls* controls)
    : controls_(controls),
      time_(0.5f, 0.0f, 1.0f),
      depth_(0.0f, 0.0f, 1.0f),
      rate_(0.5f) {}

absl::Status Chorus::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  return absl::OkStatus();
}

bool Chorus::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void Chorus::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void Chorus::ReloadParams() {
  rapidjson::Document doc;

  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  time_ = Parameter::FromJSON(controls_, doc, "time");
  depth_ = Parameter::FromJSON(controls_, doc, "depth");
  rate_ = Parameter::FromJSON(controls_, doc, "rate");

  time_.SetRange(0.0f, 1.0f);
  depth_.SetRange(0.0f, 1.0f);
}

void Chorus::Render(SampleTick tick, AudioBuffer& buffer,
                    const std::list<MidiEventAt>& /*events*/) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    chorus_params_.time_ = time_.GetValue(current_tick);
    chorus_params_.depth_ = depth_.GetValue(current_tick);
    chorus_params_.rate_ = rate_.GetValue(current_tick);

    if (!initialized_) {
      chorus_.Init(chorus_params_);
      initialized_ = true;
    } else {
      chorus_.FastUpdate(chorus_params_);
    }

    auto p = chorus_.Render(lch[i], rch[i]);
    lch[i] = p.first;
    rch[i] = p.second;
  }
}

}  // namespace fx
}  // namespace soir
