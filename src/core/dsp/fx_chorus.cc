#include <absl/log/log.h>

#include "fx_chorus.hh"

namespace soir {
namespace dsp {

FxChorus::FxChorus(dsp::Controls* controls)
    : controls_(controls), time_(0.5f), depth_(0.0f), rate_(0.5f) {}

absl::Status FxChorus::Init(const Fx::Settings& settings) {
  settings_ = settings;

  ReloadParams();

  return absl::OkStatus();
}

bool FxChorus::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void FxChorus::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void FxChorus::ReloadParams() {
  rapidjson::Document doc;

  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  time_ = Parameter::FromJSON(controls_, doc, "time");
  depth_ = Parameter::FromJSON(controls_, doc, "depth");
  rate_ = Parameter::FromJSON(controls_, doc, "rate");
}

void FxChorus::Render(SampleTick tick, AudioBuffer&) {
  std::lock_guard<std::mutex> lock(mutex_);
}

}  // namespace dsp
}  // namespace soir
