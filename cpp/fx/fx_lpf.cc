#include "fx_lpf.hh"

#include <absl/log/log.h>

#include "core/common.hh"
#include "utils/tools.hh"

namespace soir {
namespace fx {

LPF::LPF(Controls* controls)
    : controls_(controls),
      cutoff_(0.5f, 0.0f, 1.0f),
      resonance_(0.5f, 0.0f, 1.0f) {}

absl::Status LPF::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  return absl::OkStatus();
}

bool LPF::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void LPF::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void LPF::ReloadParams() {
  rapidjson::Document doc;

  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  cutoff_ = Parameter::FromJSON(controls_, doc, "cutoff");
  cutoff_.SetRange(0.0f, 1.0f);

  resonance_ = Parameter::FromJSON(controls_, doc, "resonance");
  resonance_.SetRange(0.0f, 1.0f);
}

namespace {

// Map normalized cutoff [0.0-1.0] to frequency [20Hz-20kHz]
// using MEL scale to sound more linear to human ear
float mapToFrequency(float normalized) {
  static const float min = 2595.0 * std::log10(1.0 + kMinFreq / 700.0);
  static const float max = 2595.0 * std::log10(1.0 + kMaxFreq / 700.0);

  float mel = min + normalized * (max - min);
  return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0);
}

}  // namespace

void LPF::Render(SampleTick tick, AudioBuffer& buffer,
                 const std::list<MidiEventAt>& /*events*/) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    lpf_params_.cutoff_ = mapToFrequency(cutoff_.GetValue(current_tick));
    lpf_params_.resonance_ = resonance_.GetValue(current_tick);

    lpf_left_.UpdateParameters(lpf_params_);
    lpf_right_.UpdateParameters(lpf_params_);

    lch[i] = lpf_left_.Process(lch[i]);
    rch[i] = lpf_right_.Process(rch[i]);
  }
}

}  // namespace fx
}  // namespace soir
