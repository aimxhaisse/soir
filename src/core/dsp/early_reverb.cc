#include "core/dsp/early_reverb.hh"

namespace soir {
namespace dsp {

bool operator!=(const EarlyReverb::Parameters& lhs,
                const EarlyReverb::Parameters& rhs) {
  return std::tie(lhs.time_, lhs.absorbency_) !=
         std::tie(rhs.time_, rhs.absorbency_);
}

EarlyReverb::EarlyReverb() {
  random_.Seed(0xBBAADDEE);

  // We initialize the mod phase randomly to increase the spaceness of
  // the reverb: having modulations not-in-sync on l/r makes it sound
  // slightly wider.
  for (int i = 0; i < kDelayedAPFs; ++i) {
    lAPFs_[i].SetModPhase(random_.FBetween(0.0f, 1.0f));
    rAPFs_[i].SetModPhase(random_.FBetween(0.0f, 1.0f));
  }
}

void EarlyReverb::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    params_ = p;

    // Order is important here, as the configuration of the APF line
    // depends on the delay times computed in the comb filter.
    UpdateCombFilters();
    UpdateAPFs();

    UpdateLPFs();
  }
}

void EarlyReverb::UpdateLPFs() {
  LPFParams_.coefficient_ = params_.absorbency_;

  lLPF_.UpdateParameters(LPFParams_);
  rLPF_.UpdateParameters(LPFParams_);
}

namespace {

// Temporary implementation, this produces a non-stable signal and
// leads to explosion at high delay times. However it sounds good.
float FeedbackFromTime(float time) {
  static constexpr float kMinFeedback = 0.82f;
  static constexpr float kMaxFeedback = 0.992f;

  return kMinFeedback + (kMaxFeedback - kMinFeedback) * time;
}

// This weird formula comes from Kahrs' work, who figured out how to
// compute the feedback coefficient of a comb filter to get the
// desired RT60, for the given delay time.
float CombFilterFeedback(float rt60_s, float delays) {
  return powf(10.0f, (-3.0f * delays) / (rt60_s * kSampleRate));
}

// This comes from a note from Pirkle, mentionning the APF
// coefficients following a comb filter construction should be in the
// [0.5, 0.707] range. There is also in Valhalla's reverbs a note
// about 0.707 as some sort of a magic value, likely related (however
// it allows for values outside this range).
//
// Extra note: 0.707 is 1.0/sqrt(2.0), which corresponds to a gain
// value of 1.0 in a FDN using Puckette's matrice. Above this value,
// energy is not guaranteed to be stable.
float APFFeedback(float time_factor) {
  static constexpr float kAPFFeedbackMin = 0.500f;
  static constexpr float kAPFFeedbackMax = 0.707f;

  return kAPFFeedbackMin + (kAPFFeedbackMax - kAPFFeedbackMin) * time_factor;
}

}  // namespace

void EarlyReverb::UpdateCombFilters() {

  // Comb filter delay times.
  //
  // Those delay times were chosen by hearing, with the only
  // constraint that it should be prime numbers.
  //
  // Schröeder recommands using a 1:1.5 ration between the minimum
  // value and the maximum. This is something that was tried without
  // success in a different setup, but maybe we can revisit this as
  // the issue could have been somewhere else.
  static constexpr float kDelays[kCombFilters] = {
      701.0f, 739.0f, 761.0f, 829.0f, 937.0f, 977.0f, 1009.0f, 1049.0f};

  // Scaling factors for comb filter delay times.
  //
  // Below the min value, the sound is too metallic, above it sounds
  // too spaced out. There is no clear formula here, it was picked by
  // hearing percussive sounds (which tend to sound more metallic).
  //
  // The max value is more flexible and can be used to increase the
  // delay time, in combination with the feedback increase. There is a
  // formula from Kahrs to get the RT60 from the delay time and the
  // feedback. The idea here is to set the delay coefficient from the
  // knob which will give us the delay time based on the scaling
  // factor here, and accordingly compute the feedback with the
  // formula.
  static constexpr float kDelayScaleMin = 2.37f;
  static constexpr float kDelayScaleMax = 3.0f;

  // RT60 times of the reverb in seconds.
  //
  // For now, this is not accurate at all, we need to work how to get
  // a stable signal. Those were picked to mimmic the Little Plate,
  // which has the advantage of being an OK compromise: we don't need
  // to handle those tricky super-short metallic resonnances.
  //
  // RT60 here needs to be measured, but with a pure LPF comb
  // processing, it looks like it is accurate 8-)
  static constexpr float kRT60Min = 0.25f;
  static constexpr float kRT60Max = 2.0f;

  for (int i = 0; i < kCombFilters; ++i) {
    FeedbackCombFilter::Parameters& l_params = lCombParams_[i];
    FeedbackCombFilter::Parameters& r_params = rCombParams_[i];

    const float rt60_s = kRT60Min + (kRT60Max - kRT60Min) * params_.time_;
    const float delay = kDelays[i];
    const float scale =
        kDelayScaleMin + (kDelayScaleMax - kDelayScaleMin) * params_.time_;

    l_params.max_ = delay * kDelayScaleMax;
    l_params.size_ = delay * scale;
    l_params.feedback_ = CombFilterFeedback(rt60_s, l_params.size_);

    r_params.max_ = delay * kDelayScaleMax;
    r_params.size_ = delay * scale;
    r_params.feedback_ = CombFilterFeedback(rt60_s, r_params.size_);

    lCombs_[i].UpdateParameters(l_params);
    rCombs_[i].UpdateParameters(r_params);
  }
}

void EarlyReverb::UpdateAPFs() {

  // There is no strong theory behind those values, we got them by
  // hearing.  Pirkle recommands the APF modulations to be between 1
  // and 5ms, with a rate < 1Hz, we don't follow this rule.
  //
  // From hearing, there seems to be a trade-off between mod rate and
  // mode depth, you can't get both at the same time or it sounds to
  // washy. However, going high on rate is fine if the depth is low,
  // and vice versa. Using a higher rate for a smaller depth has the
  // following theoretical advantage (just intuition and observations,
  // no formal proof here): it reduces the metallic aspect of the
  // sound, because the likelyhood that two delay times overlap is
  // null (whereas having a large mod depth will result in overlapping
  // ranges of delay times).
  //
  // Another weirdness here is, as Mauve is using a Chorus behind,
  // there seems to be interferences when we use large mod depth, as
  // if they were adding up at the same times, doubling the chorusness
  // in some way.
  //
  // We might revisit this reasoning though, it could be interesting
  // to use Pirkle's strategy.
  //
  // Another unrelated note: using 8 APF increases drastically the
  // attack of the reverb, so we only use 4 instead, which yields
  // better results.
  static constexpr float kDelaysForAPF[kDelayedAPFs] = {691.0f, 757.0f, 797.0f,
                                                        869.0f};
  static constexpr float kRatioForDelayTime = 0.0566f;

  constexpr float kLeftModRates[kDelayedAPFs] = {5.25, 8.5, 14.0, 14.75};
  constexpr float kLeftModDepth[kDelayedAPFs] = {0.001, 0.002, 0.001, 0.003};
  constexpr float kRightModRates[kDelayedAPFs] = {10.5f, 16.0f, 8.25f, 12.75f};
  constexpr float kRightModDepth[kDelayedAPFs] = {0.002, 0.003, 0.0015, 0.0026};

  for (int i = 0; i < kDelayedAPFs; ++i) {
    DelayedAPF::Parameters& l_params = lAPFParams_[i];
    DelayedAPF::Parameters& r_params = rAPFParams_[i];

    const float l_delay = (kDelaysForAPF[i] * kRatioForDelayTime);
    const float r_delay = (kDelaysForAPF[i] * kRatioForDelayTime);

    const float l_mod_depth = kLeftModDepth[i] * l_delay;
    const float l_mod_rate = kLeftModRates[i];

    const float r_mod_depth = kRightModDepth[i] * r_delay;
    const float r_mod_rate = kRightModRates[i];

    const float l_max = (l_delay + l_mod_depth * l_delay) + 1;
    const float r_max = (r_delay + r_mod_depth * r_delay) + 1;

    // Note here: we don't scale the size of modulated APF with the
    // time, for some reason I didn't get, this results in
    // interpolation glitches (even though we actually do it in the
    // comb filters without issue). We were doing it marginally in the
    // original implementation so this is not actually a big change,
    // just a reminder in case we are wondering why we don't do this
    // here.

    l_params.max_ = l_max;
    l_params.size_ = l_delay;
    l_params.depth_ = l_mod_depth;
    l_params.frequency_ = l_mod_rate;
    l_params.coef_ = APFFeedback(params_.time_);

    r_params.max_ = r_max;
    r_params.size_ = r_delay;
    r_params.depth_ = r_mod_depth;
    r_params.frequency_ = r_mod_rate;
    r_params.coef_ = APFFeedback(params_.time_);

    lAPFs_[i].UpdateParameters(l_params);
    rAPFs_[i].UpdateParameters(r_params);
  }
}

void EarlyReverb::Reset() {
  for (int i = 0; i < kCombFilters; ++i) {
    lCombs_[i].Reset();
    rCombs_[i].Reset();
  }

  for (int i = 0; i < kDelayedAPFs; ++i) {
    lAPFs_[i].Reset();
    rAPFs_[i].Reset();
  }
}

std::pair<float, float> EarlyReverb::Process(float left, float right) {
  std::pair<float, float> r = {0.0, 0.0};

  // Note here: in Darroto's algorithm, there is a weird trick here,
  // half of the filters are substracted instead of summed. This
  // results in a somewhat smoother output, because the dense parts of
  // the reverberated signal tend to be flattened. We don't do this
  // here but if we ever want to get a reverb with a super-slow ramp
  // up, this may be worth investigating.
  float l_comb = 0.0;
  float r_comb = 0.0;

  for (int i = 0; i < kCombFilters; ++i) {
    const bool stereo_trick = (i % 2) == 1;

    if (!stereo_trick) {
      l_comb += lCombs_[i].Process(left);
      r_comb += rCombs_[i].Process(right);
    } else {
      // This is a stero trick, the idea here is, if we have a plate
      // reverb, whatever is in the left channel will bounce at some
      // point and get mixed with the right channel. We aren't doing
      // anything realistic here, just trying to get some sort of
      // stereo effect.
      static constexpr float kStereoMix = 0.25f;

      l_comb +=
          lCombs_[i].Process(left * (1.0f - kStereoMix) + kStereoMix * right);
      r_comb +=
          rCombs_[i].Process(right * (1.0f - kStereoMix) + kStereoMix * left);
    }
  }

  float l_delaying = l_comb;
  float r_delaying = r_comb;

  for (int i = 0; i < kDelayedAPFs; ++i) {
    l_delaying = lAPFs_[i].Process(l_delaying);
    r_delaying = rAPFs_[i].Process(r_delaying);
  }

  r.first = lLPF_.Process(l_delaying);
  r.second = rLPF_.Process(r_delaying);

  // This was calibrated to sound similar to Soundtoys Little Plate
  // reverb.  There is likely a scientific way of getting this 100%
  // correct using transfer functions and all.
  static constexpr float kGain = 0.10f;

  r.first *= kGain;
  r.second *= kGain;

  return r;
}

}  // namespace dsp
}  // namespace soir
