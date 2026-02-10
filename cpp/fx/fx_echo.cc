#include "fx_echo.hh"

#include <absl/log/log.h>

namespace soir {
namespace fx {

Echo::Echo(Controls* controls)
    : controls_(controls),
      time_(0.2f, 0.01f, 30.0f),
      feedback_(0.3f, 0.0f, 0.99f),
      dry_(0.8f, 0.0f, 1.0f),
      wet_(0.5f, 0.0f, 1.0f) {}

absl::Status Echo::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  delay_left_.Reset();
  delay_right_.Reset();

  return absl::OkStatus();
}

bool Echo::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void Echo::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void Echo::ReloadParams() {
  rapidjson::Document doc;

  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  time_ = Parameter::FromJSON(controls_, doc, "time");
  feedback_ = Parameter::FromJSON(controls_, doc, "feedback");
  dry_ = Parameter::FromJSON(controls_, doc, "dry");
  wet_ = Parameter::FromJSON(controls_, doc, "wet");

  time_.SetRange(0.01f, 30.0f);
  feedback_.SetRange(0.0f, 0.99f);
  dry_.SetRange(0.0f, 1.0f);
  wet_.SetRange(0.0f, 1.0f);
}

void Echo::Render(SampleTick tick, AudioBuffer& buffer,
                  const std::list<MidiEventAt>& /*events*/) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    auto time_value = time_.GetValue(current_tick);
    auto feedback_value = feedback_.GetValue(current_tick);
    auto dry_value = dry_.GetValue(current_tick);
    auto wet_value = wet_.GetValue(current_tick);

    // Calculate delay size in samples based on time
    params_.size_ = time_value * kSampleRate;
    params_.max_ =
        static_cast<int>(30.0f * kSampleRate);  // Max 30 second delay

    if (!initialized_) {
      delay_left_.Init(params_);
      delay_right_.Init(params_);
      initialized_ = true;
    } else {
      delay_left_.FastUpdate(params_);
      delay_right_.FastUpdate(params_);
    }

    // Read the current delayed samples
    float delayed_left = delay_left_.Read();
    float delayed_right = delay_right_.Read();

    // Calculate output with feedback
    float out_left = lch[i] + delayed_left * feedback_value;
    float out_right = rch[i] + delayed_right * feedback_value;

    // Update the delays
    delay_left_.Update(out_left);
    delay_right_.Update(out_right);

    // Mix dry and wet signal
    lch[i] = lch[i] * dry_value + delayed_left * wet_value;
    rch[i] = rch[i] * dry_value + delayed_right * wet_value;
  }
}

}  // namespace fx
}  // namespace soir
