#include "fx_reverb.hh"

namespace soir {
namespace fx {

Reverb::Reverb(Controls* controls) : controls_(controls) {}

absl::Status Reverb::Init(const Fx::Settings& settings) {
  settings_ = settings;

  return absl::OkStatus();
}

bool Reverb::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  return true;
}

void Reverb::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  settings_ = settings;
}

void Reverb::Render(SampleTick tick, AudioBuffer&) {
  std::lock_guard<std::mutex> lock(mutex_);
}

}  // namespace fx
}  // namespace soir
