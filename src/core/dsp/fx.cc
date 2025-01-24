#include "fx.hh"

namespace soir {
namespace dsp {

Fx::Fx() {}

absl::Status Fx::Init(const FxSettings& settings) {
  settings_ = settings;

  return absl::OkStatus();
}

absl::Status Fx::Stop() {
  return absl::OkStatus();
}

bool Fx::CanFastUpdate(const FxSettings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.fx_ != settings.fx_) {
    return false;
  }

  return true;
}

void Fx::FastUpdate(const FxSettings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  settings_ = settings;
}

void Fx::Render(SampleTick tick, AudioBuffer&) {
  std::lock_guard<std::mutex> lock(mutex_);
}

}  // namespace dsp
}  // namespace soir
