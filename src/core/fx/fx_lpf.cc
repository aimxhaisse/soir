#include <absl/log/log.h>

#include "core/common.hh"
#include "core/dsp.hh"
#include "core/tools.hh"

#include "fx_lpf.hh"

namespace soir {
namespace fx {

LPF::LPF(Controls* controls) : controls_(controls), cutoff_(0.5f, 0.0f, 1.0f) {}

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
}

namespace {

float CutoffToCoefficient(float cutoff) {
  return std::exp(-2.0 * kPI * cutoff / kSampleRate);
}

}  // namespace

void LPF::Render(SampleTick tick, AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    static constexpr float kMinFreq = 20.0f;
    static constexpr float kMaxFreq = 20000.0f;

    const float cutoffFreq =
        kMinFreq + (kMaxFreq - kMinFreq) * cutoff_.GetValue(current_tick);
    lpf_params_.coefficient_ = CutoffToCoefficient(cutoffFreq);

    lpf_left_.UpdateParameters(lpf_params_);
    lpf_right_.UpdateParameters(lpf_params_);

    lch[i] = lpf_left_.Process(lch[i]);
    rch[i] = lpf_right_.Process(rch[i]);
  }
}

}  // namespace fx
}  // namespace soir
