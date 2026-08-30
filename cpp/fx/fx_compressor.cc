#include "fx_compressor.hh"

#include <absl/log/log.h>

namespace soir {
namespace fx {

Compressor::Compressor(Controls* controls, SignalBus* bus)
    : controls_(controls), bus_(bus) {}

absl::Status Compressor::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  comp_.Reset();
  comp_.FastUpdate(params_);

  return absl::OkStatus();
}

bool Compressor::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  // Changing the source only changes the bus lookup name, it is safe
  // to do so without re-initializing the DSP state.
  return true;
}

void Compressor::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void Compressor::ReloadParams() {
  auto doc = nlohmann::json::parse(settings_.extra_, nullptr, false);
  if (doc.is_discarded()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  source_ = "self";
  if (doc.contains("source") && doc["source"].is_string()) {
    source_ = doc["source"].get<std::string>();
  }

  auto from_json = [this, &doc](Parameter& param, const char* name,
                                float default_value, float min, float max) {
    param = Parameter::FromJSON(controls_, doc, name);
    if (!doc.contains(name)) {
      param.SetConstant(default_value);
    }
    param.SetRange(min, max);
  };

  from_json(threshold_, "threshold", 0.25f, 0.0f, 1.0f);
  from_json(ratio_, "ratio", 4.0f, 1.0f, 20.0f);
  from_json(attack_, "attack", 0.005f, 0.0005f, 0.5f);
  from_json(release_, "release", 0.15f, 0.01f, 2.0f);
  from_json(knee_, "knee", 6.0f, 0.0f, 12.0f);
  from_json(makeup_, "makeup", 1.0f, 0.0f, 4.0f);
  from_json(wet_, "wet", 1.0f, 0.0f, 1.0f);
}

void Compressor::Render(SampleTick tick, AudioBuffer& buffer,
                        const std::list<MidiEventAt>& /*events*/) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto lch = buffer.GetChannel(kLeftChannel);
  auto rch = buffer.GetChannel(kRightChannel);

  // Resolve the external key signal, one block behind the current one.
  const float* src_l = nullptr;
  const float* src_r = nullptr;
  if (source_ != "self") {
    const std::string signal_name =
        (source_ == "master") ? std::string(kMasterSignal) : source_;
    const Block* src = bus_->Latest(signal_name, tick);
    if (src == nullptr) {
      // Missing source (typo, or track removed): pass through
      // unchanged rather than killing the track's audio.
      WarnSourceMissing();
      return;
    }
    src_l = src->left;
    src_r = src->right;
    warned_missing_ = false;
  }

  for (int i = 0; i < buffer.Size(); ++i) {
    SampleTick current_tick = tick + i;

    params_.threshold_ = threshold_.GetValue(current_tick);
    params_.ratio_ = ratio_.GetValue(current_tick);
    params_.attack_ = attack_.GetValue(current_tick);
    params_.release_ = release_.GetValue(current_tick);
    params_.knee_ = knee_.GetValue(current_tick);
    params_.makeup_ = makeup_.GetValue(current_tick);
    params_.wet_ = wet_.GetValue(current_tick);

    comp_.FastUpdate(params_);

    auto p = src_l == nullptr
                 ? comp_.Process(lch[i], rch[i], lch[i], rch[i])
                 : comp_.Process(src_l[i], src_r[i], lch[i], rch[i]);
    lch[i] = p.first;
    rch[i] = p.second;
  }
}

void Compressor::WarnSourceMissing() {
  const absl::Time now = absl::Now();
  if (warned_missing_ && now - last_missing_warn_ < absl::Seconds(1)) {
    return;
  }

  LOG(WARNING) << "Compressor FX '" << settings_.name_
               << "': sidechain source '" << source_
               << "' is not available, passing audio through";
  warned_missing_ = true;
  last_missing_warn_ = now;
}

}  // namespace fx
}  // namespace soir
