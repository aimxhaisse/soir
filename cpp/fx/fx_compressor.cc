#include "fx_compressor.hh"

#include <absl/log/log.h>

namespace soir {
namespace fx {

namespace {
// Cadence (in blocks) at which a compressor whose source is still
// unknown re-looks it up, so a source track created after the FX is
// picked up without reloading the settings. About 107 ms at kSampleRate.
constexpr int kMissingRetryBlocks = 10;
}  // namespace

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

  ResolveSource();
}

void Compressor::ResolveSource() {
  source_signal_ = nullptr;
  if (source_ == "self" || bus_ == nullptr) {
    return;
  }

  const std::string name =
      (source_ == "master") ? std::string(kMasterSignal) : source_;
  source_signal_ = bus_->Find(name);
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
    SignalBus::Signal* source = source_signal_;
    if (source == nullptr) {
      // The source was not present when the settings were last
      // loaded; retry on a slow cadence so a source track created
      // later is picked up without reloading the settings.
      if ((tick / kBlockSize) % kMissingRetryBlocks == 0) {
        ResolveSource();
        source = source_signal_;
      }
    }
    // Copy the previous block of the source out of the bus.
    if (!SignalBus::Latest(source, tick, src_left_.data(), src_right_.data())) {
      // Missing source (typo, or track removed): pass through
      // unchanged rather than killing the track's audio. The very
      // first block has no previous block to read, so it is not a
      // missing source.
      if (tick >= kBlockSize) {
        WarnSourceMissing();
      }
      return;
    }
    src_l = src_left_.data();
    src_r = src_right_.data();
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
  if (warned_missing_) {
    return;
  }

  warned_missing_ = true;
  LOG(WARNING) << "Compressor FX '" << settings_.name_
               << "': sidechain source '" << source_
               << "' is not available, passing audio through";
}

}  // namespace fx
}  // namespace soir
