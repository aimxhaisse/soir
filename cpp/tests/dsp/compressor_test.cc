#include "dsp/compressor.hh"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace soir {
namespace dsp {

namespace {

// Run the compressor with a constant key level until the envelope has
// converged on it, using the given input level.
void Converge(Compressor& comp, float key, float input = 0.0f,
              int seconds = 2) {
  const int n = seconds * kSampleRate;
  for (int i = 0; i < n; ++i) {
    comp.Process(key, key, input, input);
  }
}

float Db(float level) { return 20.0f * std::log10(std::max(level, 1e-6f)); }

}  // namespace

TEST(CompressorTest, SilentKeyIsUnity) {
  Compressor comp;
  Converge(comp, 0.0f);

  EXPECT_NEAR(comp.GainReduction(), 0.0f, 1e-4f);

  auto p = comp.Process(0.0f, 0.0f, 0.5f, -0.5f);
  EXPECT_NEAR(p.first, 0.5f, 1e-6f);
  EXPECT_NEAR(p.second, -0.5f, 1e-6f);
}

TEST(CompressorTest, NoGainReductionBelowKneeBand) {
  Compressor comp;
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 6.0f;
  comp.FastUpdate(params);

  // Just below the band (t_db - knee/2 - 0.5 dB).
  const float t_db = Db(params.threshold_);
  const float level = std::pow(10.0f, (t_db - 3.0f - 0.5f) / 20.0f);
  Converge(comp, level);

  EXPECT_LT(comp.GainReduction(), 0.01f);

  auto p = comp.Process(level, level, 0.4f, 0.4f);
  EXPECT_NEAR(p.first, 0.4f, 1e-5f);
  EXPECT_NEAR(p.second, 0.4f, 1e-5f);
}

TEST(CompressorTest, GainReductionAboveBandMatchesRatio) {
  Compressor comp;
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 0.0f;
  comp.FastUpdate(params);

  // Fully above the threshold (hard knee), envelope at unity.
  Converge(comp, 1.0f);

  const float in_db = Db(1.0f);
  const float t_db = Db(params.threshold_);
  const float expected_gr = (in_db - t_db) * (1.0f - 1.0f / params.ratio_);
  EXPECT_NEAR(comp.GainReduction(), expected_gr, 0.01f);

  const float expected_gain = std::pow(10.0f, -expected_gr / 20.0f);
  auto p = comp.Process(1.0f, 1.0f, 1.0f, 1.0f);
  EXPECT_NEAR(p.first, expected_gain, 1e-3f);
  EXPECT_NEAR(p.second, expected_gain, 1e-3f);
}

TEST(CompressorTest, SoftKneeMatchesClosedForm) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 6.0f;

  const float t_db = Db(params.threshold_);
  const float low = t_db - params.knee_ * 0.5f;
  const float high = t_db + params.knee_ * 0.5f;
  const float slope = 1.0f - 1.0f / params.ratio_;

  // Band edge: no gain reduction at the bottom of the band.
  {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, std::pow(10.0f, low / 20.0f));
    EXPECT_NEAR(comp.GainReduction(), 0.0f, 0.01f);
  }

  // Band center: the parabola, GR(center) = slope * knee / 8.
  {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, params.threshold_);
    EXPECT_NEAR(comp.GainReduction(), slope * params.knee_ / 8.0f, 0.01f);
  }

  // Top of the band: matches the above-the-band branch.
  {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, std::pow(10.0f, high / 20.0f));
    const float expected_gr = (high - t_db) * slope;
    EXPECT_NEAR(comp.GainReduction(), expected_gr, 0.01f);
  }
}

TEST(CompressorTest, SoftKneeIsContinuous) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 6.0f;
  params.attack_ = 0.005f;
  params.release_ = 0.02f;

  const float t_db = Db(params.threshold_);
  const float low = t_db - params.knee_ * 0.5f;
  const float high = t_db + params.knee_ * 0.5f;
  const float slope = 1.0f - 1.0f / params.ratio_;

  // Closed-form GR curve for these parameters.
  auto closed_form = [&](float db) {
    if (db <= low) {
      return 0.0f;
    }
    if (db >= high) {
      return (db - t_db) * slope;
    }
    const float d = db - low;
    return slope * d * d / (2.0f * params.knee_);
  };

  Compressor comp;
  comp.FastUpdate(params);

  // Sweep the key level up then down in 0.25 dB steps, settling the
  // envelope at each step (no reset: the envelope carries over). The
  // converged GR must match the closed-form curve everywhere: the
  // branches glue together without a jump (C0) across the whole band.
  const int kSteps = 40;
  const float start_db = -45.0f;
  const float step_db = 0.25f;
  const int settle = kSampleRate / 20;  // 50 ms.

  for (int pass = 0; pass < 2; ++pass) {
    for (int i = 0; i < kSteps; ++i) {
      const int index = (pass == 0) ? i : (kSteps - 1 - i);
      const float db = start_db + index * step_db;
      const float level = std::pow(10.0f, db / 20.0f);

      for (int s = 0; s < settle; ++s) {
        comp.Process(level, level, 0.0f, 0.0f);
      }

      EXPECT_NEAR(comp.GainReduction(), closed_form(db), 0.05f)
          << "GR deviates from the closed form at " << db << " dB";
    }
  }
}

TEST(CompressorTest, SoftKneeSlopeIsContinuousAtEdges) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 6.0f;

  const float t_db = Db(params.threshold_);
  const float low = t_db - params.knee_ * 0.5f;
  const float high = t_db + params.knee_ * 0.5f;
  const float slope = 1.0f - 1.0f / params.ratio_;
  const float delta = 0.1f;  // dB.

  auto gr_at = [&](float db) {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, std::pow(10.0f, db / 20.0f));
    return comp.GainReduction();
  };

  // Bottom edge: slope 0 on both sides (GR is 0 below, quadratic in).
  EXPECT_NEAR(gr_at(low - delta), 0.0f, 0.01f);
  EXPECT_LT(gr_at(low + delta), 0.01f);

  // Top edge: slope (1 - 1/ratio) on both sides.
  const float gr_high = gr_at(high);
  const float gr_above = gr_at(high + delta);
  const float gr_below = gr_at(high - delta);
  EXPECT_NEAR(gr_above - gr_high, slope * delta, 0.02f);
  EXPECT_NEAR(gr_high - gr_below, slope * delta, 0.02f);
}

TEST(CompressorTest, HardKnee) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 0.0f;

  const float t_db = Db(params.threshold_);
  const float slope = 1.0f - 1.0f / params.ratio_;

  auto gr_at = [&](float level) {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, level);
    return comp.GainReduction();
  };

  // Just below the threshold: no gain reduction.
  const float below = std::pow(10.0f, (t_db - 0.2f) / 20.0f);
  EXPECT_NEAR(gr_at(below), 0.0f, 0.01f);

  // Just above the threshold: GR follows the ratio.
  const float above = std::pow(10.0f, (t_db + 0.2f) / 20.0f);
  EXPECT_NEAR(gr_at(above), 0.2f * slope, 0.01f);
}

TEST(CompressorTest, AttackTiming) {
  Compressor::Parameters params;
  params.threshold_ = 0.01f;
  params.ratio_ = 20.0f;
  params.knee_ = 0.0f;
  params.attack_ = 0.01f;
  params.release_ = 0.02f;

  Compressor comp;
  comp.FastUpdate(params);
  comp.Reset();

  const float t_db = Db(params.threshold_);
  const float slope = 1.0f - 1.0f / params.ratio_;

  // GR as a function of the envelope level (in = 1.0 throughout).
  auto gr_of_env = [&](float env) {
    const float in_db = Db(env);
    return (in_db - t_db) * slope;
  };

  // Key steps from 0 to 1: the envelope should reach ~63% of the key
  // (1 - e^-1) after one attack time constant.
  const float target_gr = gr_of_env(1.0f - std::exp(-1.0f));

  int n = 0;
  const int limit = kSampleRate;  // 1 second.
  for (; n < limit; ++n) {
    const float key = (n < kSampleRate / 2) ? 0.0f : 1.0f;
    comp.Process(key, key, 1.0f, 1.0f);
    if (n >= kSampleRate / 2 && comp.GainReduction() >= target_gr) {
      break;
    }
  }

  const int attack_samples = n - kSampleRate / 2;
  const int expected = static_cast<int>(params.attack_ * kSampleRate);
  EXPECT_GT(attack_samples, 0);
  EXPECT_LT(attack_samples, limit);
  EXPECT_GT(attack_samples, expected / 2);
  EXPECT_LT(attack_samples, expected * 3 / 2);
}

TEST(CompressorTest, ReleaseTiming) {
  Compressor::Parameters params;
  params.threshold_ = 0.01f;
  params.ratio_ = 20.0f;
  params.knee_ = 0.0f;
  params.attack_ = 0.005f;
  params.release_ = 0.02f;

  Compressor comp;
  comp.FastUpdate(params);

  // Converge the envelope on a unity key, then drop the key to 0:
  // the envelope should decay to ~37% (e^-1) after one release time
  // constant.
  Converge(comp, 1.0f);

  const float t_db = Db(params.threshold_);
  const float slope = 1.0f - 1.0f / params.ratio_;
  const float target_gr = (Db(std::exp(-1.0f)) - t_db) * slope;

  int n = 0;
  const int limit = kSampleRate;  // 1 second.
  for (; n < limit; ++n) {
    comp.Process(0.0f, 0.0f, 1.0f, 1.0f);
    if (comp.GainReduction() <= target_gr) {
      break;
    }
  }

  const int expected = static_cast<int>(params.release_ * kSampleRate);
  EXPECT_GT(n, 0);
  EXPECT_LT(n, limit);
  EXPECT_GT(n, expected / 2);
  EXPECT_LT(n, expected * 3 / 2);
}

TEST(CompressorTest, MakeupGain) {
  Compressor::Parameters params;
  params.makeup_ = 2.0f;

  Compressor comp;
  comp.FastUpdate(params);
  Converge(comp, 0.0f);

  auto p = comp.Process(0.0f, 0.0f, 0.5f, 0.5f);
  EXPECT_NEAR(p.first, 1.0f, 1e-5f);
  EXPECT_NEAR(p.second, 1.0f, 1e-5f);
}

TEST(CompressorTest, DryWetBlend) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 0.0f;

  const float t_db = Db(params.threshold_);
  const float gr = (Db(1.0f) - t_db) * (1.0f - 1.0f / params.ratio_);
  const float wet_out = std::pow(10.0f, -gr / 20.0f);

  // Fully wet: compressed output.
  {
    Compressor comp;
    params.wet_ = 1.0f;
    comp.FastUpdate(params);
    Converge(comp, 1.0f);
    auto p = comp.Process(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_NEAR(p.first, wet_out, 1e-3f);
  }

  // Fully dry: untouched input.
  {
    Compressor comp;
    params.wet_ = 0.0f;
    comp.FastUpdate(params);
    Converge(comp, 1.0f);
    auto p = comp.Process(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_NEAR(p.first, 1.0f, 1e-5f);
  }

  // 50/50: the midpoint of dry and wet.
  {
    Compressor comp;
    params.wet_ = 0.5f;
    comp.FastUpdate(params);
    Converge(comp, 1.0f);
    auto p = comp.Process(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_NEAR(p.first, (wet_out + 1.0f) / 2.0f, 1e-3f);
  }
}

TEST(CompressorTest, StereoKeyTakesMax) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;
  params.knee_ = 0.0f;

  const float t_db = Db(params.threshold_);

  // Loud key on the right only: the linked envelope still engages.
  {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, 0.8f);
    for (int i = 0; i < 10; ++i) {
      comp.Process(0.8f, 0.0f, 1.0f, 1.0f);
    }
    const float expected_gr = (Db(0.8f) - t_db) * (1.0f - 1.0f / 4.0f);
    EXPECT_NEAR(comp.GainReduction(), expected_gr, 0.01f);
  }

  // Silent key on both channels: no gain reduction.
  {
    Compressor comp;
    comp.FastUpdate(params);
    Converge(comp, 0.0f);
    auto p = comp.Process(0.0f, 0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(comp.GainReduction(), 0.0f, 1e-4f);
    EXPECT_NEAR(p.first, 1.0f, 1e-5f);
  }
}

TEST(CompressorTest, ResetClearsState) {
  Compressor::Parameters params;
  params.threshold_ = 0.25f;
  params.ratio_ = 4.0f;

  Compressor comp;
  comp.FastUpdate(params);
  Converge(comp, 1.0f);
  EXPECT_GT(comp.GainReduction(), 1.0f);

  comp.Reset();
  EXPECT_NEAR(comp.GainReduction(), 0.0f, 1e-6f);

  auto p = comp.Process(0.0f, 0.0f, 0.5f, 0.5f);
  EXPECT_NEAR(p.first, 0.5f, 1e-6f);
}

}  // namespace dsp
}  // namespace soir
