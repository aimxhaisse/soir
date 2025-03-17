#pragma once

#include "fx.hh"

namespace soir {
namespace dsp {

// Chorus effect.
struct FxChorus : public Fx {
  absl::Status Init(const Fx::Settings& settings) override;

  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer&) override;

 private:
  std::mutex mutex_;
  Fx::Settings settings_;
};

}  // namespace dsp
}  // namespace soir
