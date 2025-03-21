#include "fx_reverb.hh"

namespace soir {


FxReverb::FxReverb(Controls* controls) : controls_(controls) {}

absl::Status FxReverb::Init(const Fx::Settings& settings) {
  settings_ = settings;

  return absl::OkStatus();
}

bool FxReverb::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void FxReverb::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  settings_ = settings;
}

void FxReverb::Render(SampleTick tick, AudioBuffer&) {
  std::lock_guard<std::mutex> lock(mutex_);
}


}  // namespace soir
