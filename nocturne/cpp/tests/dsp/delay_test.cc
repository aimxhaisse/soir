#include "dsp/comb_filter.hh"
#include "dsp/delay.hh"
#include "dsp/delayed_apf.hh"
#include "dsp/lfo.hh"
#include "dsp/modulated_delay.hh"

#include <gtest/gtest.h>

namespace soir {
namespace dsp {

TEST(DelayTest, Construction) {
  Delay delay;
  EXPECT_TRUE(true);
}

TEST(DelayTest, ProcessSample) {
  Delay delay;
  Delay::Parameters params;
  params.max_ = 100;
  params.size_ = 10.0f;

  delay.Init(params);

  // Feed some samples
  for (int i = 0; i < 20; i++) {
    float output = delay.Render(static_cast<float>(i));
    EXPECT_TRUE(std::isfinite(output));
  }
}

TEST(LFOTest, Construction) {
  LFO lfo;
  EXPECT_TRUE(true);
}

TEST(LFOTest, RenderSamples) {
  LFO lfo;
  LFO::Parameters params;
  params.type_ = LFO::SINE;
  params.frequency_ = 1.0f;

  lfo.Init(params);

  for (int i = 0; i < 100; i++) {
    float value = lfo.Render();
    EXPECT_TRUE(std::isfinite(value));
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(FeedbackCombFilterTest, Construction) {
  FeedbackCombFilter filter;
  EXPECT_TRUE(true);
}

TEST(FeedbackCombFilterTest, ProcessSample) {
  FeedbackCombFilter filter;
  FeedbackCombFilter::Parameters params;
  params.max_ = 100;
  params.size_ = 50.0f;
  params.feedback_ = 0.5f;

  filter.Init(params);

  float output = filter.Process(0.5f);
  EXPECT_TRUE(std::isfinite(output));
}

TEST(ModulatedDelayTest, Construction) {
  ModulatedDelay delay;
  EXPECT_TRUE(true);
}

TEST(ModulatedDelayTest, ProcessSample) {
  ModulatedDelay delay;
  ModulatedDelay::Parameters params;
  params.max_ = 1000;
  params.size_ = 100.0f;
  params.depth_ = 10.0f;
  params.frequency_ = 1.0f;
  params.type_ = LFO::SINE;

  delay.Init(params);

  float output = delay.Render(0.5f);
  EXPECT_TRUE(std::isfinite(output));
}

TEST(DelayedAPFTest, Construction) {
  DelayedAPF apf;
  EXPECT_TRUE(true);
}

TEST(DelayedAPFTest, ProcessSample) {
  DelayedAPF apf;
  DelayedAPF::Parameters params;
  params.max_ = 1000;
  params.size_ = 100.0f;
  params.depth_ = 10.0f;
  params.frequency_ = 1.0f;
  params.coef_ = 0.7f;

  apf.UpdateParameters(params);

  float output = apf.Process(0.5f);
  EXPECT_TRUE(std::isfinite(output));
}

}  // namespace dsp
}  // namespace soir
