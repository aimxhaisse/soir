#pragma once

#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace fx {

// Chorus effect.
struct Chorus : public Fx {
  Chorus(Controls* controls);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer&) override;

 private:
  void ReloadParams();

  Controls* controls_;

  std::mutex mutex_;
  Fx::Settings settings_;

  Parameter time_;
  Parameter depth_;
  Parameter rate_;
};

}  // namespace fx
}  // namespace soir
