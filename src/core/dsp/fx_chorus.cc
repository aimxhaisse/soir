#include "fx_chorus.hh"

namespace soir {
namespace dsp {

absl::Status FxChorus::Init(const Fx::Settings& settings) {
  settings_ = settings;

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

  settings_ = settings;
}

void FxChorus::Render(SampleTick tick, AudioBuffer&) {
  std::lock_guard<std::mutex> lock(mutex_);
}

}  // namespace dsp
}  // namespace soir
