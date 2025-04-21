#pragma once

#include "core/dsp/high_pass_filter.hh"
#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace fx {

// High Pass Filter effect.
struct HPF : public Fx {
  HPF(Controls* controls);

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

  dsp::HighPassFilter::Parameters hpf_params_;
  dsp::HighPassFilter hpf_left_;
  dsp::HighPassFilter hpf_right_;
};

}  // namespace fx
}  // namespace soir