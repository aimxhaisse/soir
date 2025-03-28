#pragma once

#include <array>

#include "core/dsp/comb_filter.hh"
#include "core/dsp/delayed_apf.hh"
#include "core/fast_random.hh"

namespace soir {
namespace dsp {

// Reverb engine for early reverberations.
//
// Parallel comb filter are chained with a line of modulated APF
// filters. The output sound is slightly metallic, despite a lot of
// effort trying to reduce it without compromises. It however sounds
// generally OK.
//
// Though some numbers may look random here, there is quite some
// tuning around it. Multiple attempts were made, following research
// papers etc, not often producing good results, before having this
// sort of compromise.
class EarlyReverb {
 public:
  static constexpr int kCombFilters = 8;
  static constexpr int kDelayedAPFs = 4;

  // We try to keep this structure as simple as possible, not for
  // computations but to be able to re-use it. Former implementations
  // were describing all parameters, but this ended up being a mess as
  // the logic behind was spread in multiple files.
  struct Parameters {
    float time_ = 0.5f;

    // Here we configure the LPF coefficients, this value can be
    // tweaked until it sounds good. It has to be lower than the one
    // used in the late reverbs, so that we can a nice progressive
    // damping towards mid frequencies.
    float absorbency_ = 0.18f;
  };

  EarlyReverb();

  void UpdateParameters(const Parameters& p);
  void Reset();

  std::pair<float, float> Process(float left, float right);

 private:
  void UpdateCombFilters();
  void UpdateAPFs();
  void UpdateLPFs();

  Parameters params_;

  // Used to initialize the mod phases at somewhat random positions.
  FastRandom random_;

  // Parallel comb filters.
  FeedbackCombFilter lCombs_[kCombFilters];
  FeedbackCombFilter rCombs_[kCombFilters];
  FeedbackCombFilter::Parameters lCombParams_[kCombFilters];
  FeedbackCombFilter::Parameters rCombParams_[kCombFilters];

  // Line of Delayed APFs.
  DelayedAPF lAPFs_[kDelayedAPFs];
  DelayedAPF rAPFs_[kDelayedAPFs];
  DelayedAPF::Parameters lAPFParams_[kDelayedAPFs];
  DelayedAPF::Parameters rAPFParams_[kDelayedAPFs];

  // This is a post-hack, we added this at the very end of the
  // implementation, as it sounded a bit too bright, and sometimes
  // resonnance would appear in high frequencies.
  LPF1P::Parameters LPFParams_;
  LPF1P lLPF_;
  LPF1P rLPF_;
};

}  // namespace dsp
}  // namespace soir
