#pragma once

#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace dsp {

// Reverb effect.
struct FxReverb : public Fx {
  FxReverb(dsp::Controls* controls);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer&) override;

 private:
  dsp::Controls* controls_;

  std::mutex mutex_;
  Fx::Settings settings_;
};

}  // namespace dsp
}  // namespace soir
