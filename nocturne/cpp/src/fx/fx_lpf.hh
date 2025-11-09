#pragma once

#include "dsp/low_pass_filter.hh"
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
  Parameter resonance_;

  dsp::LowPassFilter::Parameters lpf_params_;
  dsp::LowPassFilter lpf_left_;
  dsp::LowPassFilter lpf_right_;
};

}  // namespace fx
}  // namespace soir
