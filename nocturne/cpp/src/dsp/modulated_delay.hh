#pragma once

#include <tuple>

#include "dsp/delay.hh"
#include "dsp/dsp.hh"
#include "dsp/lfo.hh"

namespace soir {
namespace dsp {

class ModulatedDelay {
 public:
  struct Parameters {
    // Maximum size to expect in samples, this must account for both
    // increases in size as well as depth. Not doing so will result in
    // clipping or crashes.
    int max_ = 1;

    // Size of the delay in samples.
    float size_ = 1.0f;

    // Depth of the delay in samples. The actual delay oscillates
    // around the size by up to this amount.
    float depth_ = 0;

    float frequency_ = 0.5f;
    LFO::Type type_ = LFO::SINE;
    Delay::Interpolation interpolation_ = Delay::LAGRANGE;
  };

  ModulatedDelay();

  // Initialize with the given parameters
  void Init(const Parameters& p);

  // Fast update to parameters that don't require full reinitialization
  void FastUpdate(const Parameters& p);

  // Updates the modulation, reads the state before updating it.
  float Render(float xn);

  // When using multiple delays in parallel, it is a good idea to seed
  // them with a different phase at the beginning, so that we end up
  // with some randomization.
  void SetModPhase(float phase);

  // Alternative way to decouple steps in process, this can be used in
  // case you want to feed something that depends on the delayed
  // sample (i.e: feedback or so).
  void UpdateMod();
  float Read();
  void UpdateState(float xn);

  // Empty the delay, mainly used when changing position in DAW.
  void Reset();

 private:
  void InitFromParameters();

  Delay delay_;
  LFO lfo_;
  Parameters params_;
  float mod_ = 0.0f;

  // Those are refreshed whenever the modulated delay params are
  // updated.
  LFO::Parameters lfo_params_;
  Delay::Parameters delay_params_;
};

inline bool operator!=(const ModulatedDelay::Parameters& lhs,
                       const ModulatedDelay::Parameters& rhs) {
  return std::tie(lhs.max_, lhs.size_, lhs.depth_, lhs.frequency_, lhs.type_,
                  lhs.interpolation_) !=
         std::tie(rhs.max_, rhs.size_, rhs.depth_, rhs.frequency_, rhs.type_,
                  rhs.interpolation_);
}

}  // namespace dsp
}  // namespace soir
