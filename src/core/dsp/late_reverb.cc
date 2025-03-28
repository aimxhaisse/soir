#include "core/dsp/late_reverb.hh"

namespace soir {
namespace dsp {

bool operator!=(const LateReverb::Parameters& lhs,
                const LateReverb::Parameters& rhs) {
  return std::tie(lhs.time_, lhs.absorbency_) !=
         std::tie(rhs.time_, rhs.absorbency_);
}

LateReverb::LateReverb(MatrixFlavor flavor)
    : flavor_(flavor),
      size_(flavor == M4x4 ? 4 : 16),
      matrixSize_(size_ * size_) {
  lineParams_.resize(size_);
  lLines_.resize(size_);
  rLines_.resize(size_);
  matrix_.resize(matrixSize_);
  LPFParams_.resize(size_);
  lLPFs_.resize(size_);
  rLPFs_.resize(size_);
  lDelayedValues_.resize(size_);
  rDelayedValues_.resize(size_);

  random_.Seed(0x11133777);

  for (int i = 0; i < size_; ++i) {
    lLines_[i].SetModPhase(random_.FBetween(0.0f, 1.0f));
    rLines_[i].SetModPhase(random_.FBetween(0.0f, 1.0f));
  }

  MakeMatrix();

  Reset();
}

void LateReverb::MakeMatrix() {
  switch (flavor_) {
    case M4x4:
      matrix_ = {
          0.0,  1.0, 1.0,  0.0,   // dummy comment to
          -1.0, 0.0, 0.0,  -1.0,  // keep this matrix
          1.0,  0.0, 0.0,  -1.0,  // in a shape that
          0.0,  1.0, -1.0, 0.0    // looks good.
      };
      break;

    case M16x16:
      // Based on Householder coefficients.
      //
      // More about this matrix construct can be found in the hybrid
      // reverb master thesis (p. 14). The idea is to build a 16x16 matrix
      // from the 4x4 base matrix.
      static constexpr std::array<float, 16> kBaseMatrix = {
          1.0,  -1.0, -1.0, -1.0,  // dummy comment to
          -1.0, 1.0,  -1.0, -1.0,  // keep this matrix
          -1.0, -1.0, 1.0,  -1.0,  // in a shape that
          -1.0, -1.0, -1.0, 1.0    // looks good.
      };

      for (int line = 0; line < 16; ++line) {
        for (int column = 0; column < 16; ++column) {
          const float c = kBaseMatrix[(line / 4) * 4 + column / 4];
          matrix_[line * 16 + column] =
              kBaseMatrix[(line % 4) * 4 + column % 4] * c;
        }
      }

      for (auto& v : matrix_) {
        v /= 4.0;
      }

      break;
  }
}

void LateReverb::Reset() {
  for (int i = 0; i < size_; ++i) {
    lLines_[i].Reset();
    rLines_[i].Reset();

    lDelayedValues_[i] = 0.0f;
    rDelayedValues_[i] = 0.0f;
  }
}

void LateReverb::UpdateParameters(const Parameters& p) {
  if (p != params_) {
    // Important to set this here as some methods used below depend on
    // the internal sample rate to be up-to-date.
    params_ = p;

    // DelayInSamples are based on prime numbers, this is to reduce
    // the resonnance effect which happens when a signal with a lower
    // frequency than the delay is repeatedly delayed. This doesn't
    // happen if the delays are all primes. As we modulate delay, it
    // still happens, but the likelyhood is slightly decreased.
    struct Config {
      float delay_ = 0.0f;
      float modDepth_ = 0.0f;
      float modRate_ = 0.0f;
    };

    static const Config kConfig4x4[] = {
        Config{2053.0f, 8.30, 0.27},   //@
        Config{2437.0f, 12.50, 0.39},  //@
        Config{2719.0f, 13.80, 0.43},  //@
        Config{3169.0f, 24.90, 0.23},  //@
    };

    static const Config kConfig16x16[] = {
        Config{2053.0f, 8.30, 0.27},   //@
        Config{2111.0f, 9.30, 0.30},   //@
        Config{2213.0f, 10.30, 0.25},  //@
        Config{2333.0f, 11.30, 0.21},  //@

        Config{2437.0f, 12.50, 0.37},  //@
        Config{2521.0f, 13.50, 0.32},  //@
        Config{2579.0f, 14.50, 0.35},  //@
        Config{2621.0f, 15.50, 0.41},  //@

        Config{2719.0f, 14.80, 0.40},  //@
        Config{2767.0f, 15.80, 0.43},  //@
        Config{2801.0f, 16.80, 0.47},  //@
        Config{2903.0f, 17.80, 0.38},  //@

        Config{3169.0f, 25.90, 0.20},  //@
        Config{3221.0f, 26.90, 0.22},  //@
        Config{3313.0f, 27.90, 0.23},  //@
        Config{3413.0f, 28.90, 0.29},  //@
    };

    const Config* config = (flavor_ == M4x4) ? kConfig4x4 : kConfig16x16;

    // Scaling factors for delay times. This is linearly scaled by the
    // time factor. We need to experiment with these values so they
    // fit well with the early reverberations.
    static constexpr float kDelayScaleMin = 0.85f;
    static constexpr float kDelayScaleMax = 1.30f;

    const float scale_time =
        kDelayScaleMin + params_.time_ * (kDelayScaleMax - kDelayScaleMin);

    for (int i = 0; i < size_; ++i) {
      ModulatedDelay::Parameters& p = lineParams_[i];
      const Config& c = config[i];

      p.max_ = kDelayScaleMax * c.delay_ + c.modDepth_ + 1;
      p.size_ = scale_time * c.delay_;
      p.frequency_ = c.modRate_;
      p.depth_ = c.modDepth_;

      // We use the same parameters for both channels, maybe we could
      // investigate how it sounds if we use slightly different delay
      // times. One difference though is that we randomly assign the
      // modulation phase.
      lLines_[i].FastUpdate(p);
      rLines_[i].FastUpdate(p);

      LPFParams_[i].coefficient_ = params_.absorbency_;

      lLPFs_[i].UpdateParameters(LPFParams_[i]);
      rLPFs_[i].UpdateParameters(LPFParams_[i]);
    }
  }
}

namespace {

// This weird formula comes from Kahrs' work, who figured out how to
// compute the feedback coefficient of a comb filter to get the
// desired RT60, for the given delay time. It should be noted that
// this comes from Comb filter feedback, however it seems to work well
// here, once divided by sqrt(2.0).
float fdnFeedback(float rt60_s, float delays) {
  static constexpr float kSqrtOfTwo = 1.41421356237309515f;

  return powf(10.0f, (-3.0f * delays) / (rt60_s * kSampleRate));
}

float gainFor(LateReverb::MatrixFlavor flavor) {
  // This was calibrated to sound similar to Soundtoys Little Plate
  // reverb.  There is likely a scientific way of getting this 100%
  // correct using transfer functions and all.
  switch (flavor) {
    case LateReverb::M4x4:
    case LateReverb::M16x16:
      return 0.14f;
  }

  return 0.0f;
}

}  // namespace

std::pair<float, float> LateReverb::Process(float lxn, float rxn) {
  // Compute result first, which is the sum of whatever is in the
  // modulated lines. We also store a copy of each line in
  // lDelayedValue so we can use it to update the state of the
  // connected delays in a second time.
  float l_result = 0.0f;
  float r_result = 0.0f;

  for (int i = 0; i < size_; ++i) {
    const float l = lLPFs_[i].Process(lLines_[i].Read());
    const float r = rLPFs_[i].Process(rLines_[i].Read());

    lDelayedValues_[i] = l;
    rDelayedValues_[i] = r;

    l_result += l;
    r_result += r;
  }

  // RT60 times of the reverb in seconds.
  //
  // For now, this is not accurate at all, we need to work how to get
  // a stable signal.
  //
  // RT60 here needs to be measured, but with a pure LPF comb
  // processing, it looks like it is accurate 8-)
  //
  // This is twice as long as the early, which is fine as they scale
  // linearly with time_.
  static constexpr float kRT60Min = 0.5f;
  static constexpr float kRT60Max = 60.0f;

  const float rt60_s = kRT60Min + (kRT60Max - kRT60Min) * params_.time_;

  // Actual update of each line of modulated delay.
  for (int i = 0; i < size_; ++i) {
    float l_sum = lxn;
    float r_sum = rxn;

    for (int peek = 0; peek < size_; ++peek) {
      const float fb = fdnFeedback(rt60_s, lineParams_[i].size_);
      const float coef = matrix_[i * size_ + peek] * fb;

      l_sum += coef * lDelayedValues_[peek];
      r_sum += coef * rDelayedValues_[peek];
    }

    lLines_[i].UpdateMod();
    lLines_[i].UpdateState(l_sum);

    rLines_[i].UpdateMod();
    rLines_[i].UpdateState(r_sum);
  }

  const float gain = gainFor(flavor_);

  l_result *= gain;
  r_result *= gain;

  return {l_result, r_result};
}

}  // namespace dsp
}  // namespace soir
