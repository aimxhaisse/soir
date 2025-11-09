#pragma once

#include "dsp/delay.hh"
#include "core/parameter.hh"

#include "fx.hh"

namespace soir {
namespace fx {

// Echo effect with feedback.
struct Echo : public Fx {
  Echo(Controls* controls);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer& buffer) override;

 private:
  void ReloadParams();

  Controls* controls_;

  std::mutex mutex_;
  Fx::Settings settings_;

  Parameter time_;       // Delay time in seconds
  Parameter feedback_;   // Feedback amount (0.0-1.0)
  Parameter dry_;        // Dry level
  Parameter wet_;        // Wet level

  bool initialized_ = false;

  dsp::Delay::Parameters params_;
  dsp::Delay delay_left_;
  dsp::Delay delay_right_;
};

}  // namespace fx
}  // namespace soir