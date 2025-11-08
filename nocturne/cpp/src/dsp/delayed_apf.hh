#pragma once

#include "dsp/delay.hh"
#include "dsp/lfo.hh"
#include "dsp/lpf.hh"
#include "dsp/modulated_delay.hh"

namespace soir {
namespace dsp {

// This is a delayed APF with a modulated time delay revolving around
// a sine wave.
class DelayedAPF {
 public:
  struct Parameters {
    // Maximum size of the delay in samples, need to account for the
    // depth as well.
    int max_ = 1;

    // Current size of the delay in samples, can be changed
    // dynamically and interpolation will happen.
    float size_ = 1.0f;

    // Depth of the delay in samples. The actual delay oscillates
    // around the size by up to this amount.
    float depth_ = 0.0f;

    // Frequency of the modulation in hertz.
    float frequency_ = 0.5f;

    // Type of modulation.
    LFO::Type type_ = LFO::SINE;

    // Feedback coefficient of the APF.
    float coef_ = 0.0f;

    // Mix of the input signal.
    float mix_ = 1.0f;

    // Coefficient of the LPF filter.
    float lpf_ = 0.2f;

    // Interpolation to use whenever the time parameter changes.
    Delay::Interpolation interpolation_ = Delay::LAGRANGE;
  };

  DelayedAPF();

  // Updates parameters if they changed since the last call. No-op if
  // they are the same.
  void UpdateParameters(const Parameters& p);

  // Retrieves a sample at position size and updates the state.
  float Process(float xn);

  // When using multiple APF in parallel, it is a good idea to seed
  // them with a different phase at the beginning, so that we end up
  // with some randomization.
  void SetModPhase(float coef);

  void Reset();

 private:
  void InitFromParameters();

  // This is to get a somewhat stable signal, values here should
  // probably range between 0.0 and 0.5 (source: Pirkle). This is
  // called "Damping" in some reverbs.
  LPF1P lpf_;
  LPF1P::Parameters lpfParams_;

  // The actual modulated delay.
  Parameters params_;
  ModulatedDelay::Parameters delayParams_;
  ModulatedDelay delay_;

  LFO::Parameters lfoParams_;
  LFO lfo_;
};

inline bool operator!=(const DelayedAPF::Parameters& lhs,
                       const DelayedAPF::Parameters& rhs) {
  return std::tie(lhs.max_, lhs.size_, lhs.depth_, lhs.frequency_, lhs.type_,
                  lhs.coef_, lhs.mix_, lhs.lpf_, lhs.interpolation_) !=
         std::tie(rhs.max_, rhs.size_, rhs.depth_, rhs.frequency_, rhs.type_,
                  rhs.coef_, rhs.mix_, rhs.lpf_, rhs.interpolation_);
}

}  // namespace dsp
}  // namespace soir
