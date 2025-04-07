#pragma once

#include "core/dsp/lpf.hh"
#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace fx {

// Low Pass Filter effect.
struct LPF : public Fx {
  LPF(Controls* controls);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer&) override;

 private:
  void ReloadParams();

  Controls* controls_;

  std::mutex mutex_;
  Fx::Settings settings_;

  Parameter cutoff_;

  dsp::LPF1P::Parameters lpf_params_;
  dsp::LPF1P lpf_left_;
  dsp::LPF1P lpf_right_;
};

}  // namespace fx
}  // namespace soir
