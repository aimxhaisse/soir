#pragma once

#include "core/dsp/reverb.hh"
#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace fx {

// Reverb effect.
struct Reverb : public Fx {
  Reverb(Controls* controls);

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
  Parameter dry_;
  Parameter wet_;

  bool initialized_ = false;

  dsp::Reverb::Parameters params_;
  dsp::Reverb reverb_;
};

}  // namespace fx
}  // namespace soir
