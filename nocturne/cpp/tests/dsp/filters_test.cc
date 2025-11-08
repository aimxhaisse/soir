#include "dsp/band_pass_filter.hh"
#include "dsp/biquad_filter.hh"
#include "dsp/high_pass_filter.hh"
#include "dsp/low_pass_filter.hh"
#include "dsp/lpf.hh"

#include <gtest/gtest.h>

namespace soir {
namespace dsp {

TEST(BiquadFilterTest, Construction) {
  BiquadFilter filter;
  EXPECT_TRUE(true);
}

TEST(BiquadFilterTest, ProcessSample) {
  BiquadFilter filter;
  BiquadFilter::Parameters params;
  params.a0_ = 1.0f;
  params.a1_ = 0.0f;
  params.a2_ = 0.0f;
  params.b1_ = 0.0f;
  params.b2_ = 0.0f;

  filter.UpdateParameters(params);

  // With a0=1 and all others 0, output should equal input
  float input = 0.5f;
  float output = filter.Process(input);
  EXPECT_NEAR(output, input, 0.001f);
}

TEST(LowPassFilterTest, Construction) {
  LowPassFilter filter;
  EXPECT_TRUE(true);
}

TEST(LowPassFilterTest, ProcessSample) {
  LowPassFilter filter;
  LowPassFilter::Parameters params;
  params.cutoff_ = 1000.0f;
  params.resonance_ = 0.5f;

  filter.UpdateParameters(params);

  float output = filter.Process(0.5f);
  // Just verify it doesn't crash and returns a finite value
  EXPECT_TRUE(std::isfinite(output));
}

TEST(HighPassFilterTest, Construction) {
  HighPassFilter filter;
  EXPECT_TRUE(true);
}

TEST(HighPassFilterTest, ProcessSample) {
  HighPassFilter filter;
  HighPassFilter::Parameters params;
  params.cutoff_ = 1000.0f;
  params.resonance_ = 0.5f;

  filter.UpdateParameters(params);

  float output = filter.Process(0.5f);
  EXPECT_TRUE(std::isfinite(output));
}

TEST(BandPassFilterTest, Construction) {
  BandPassFilter filter;
  EXPECT_TRUE(true);
}

TEST(BandPassFilterTest, ProcessSample) {
  BandPassFilter filter;
  BandPassFilter::Parameters params;
  params.frequency_ = 1000.0f;
  params.width_coefficient_ = 0.5f;
  params.boost_db_ = 0.0f;

  filter.UpdateParameters(params);

  float output = filter.Process(0.5f);
  EXPECT_TRUE(std::isfinite(output));
}

TEST(LPF1PTest, Construction) {
  LPF1P filter;
  EXPECT_TRUE(true);
}

TEST(LPF1PTest, ProcessSample) {
  LPF1P filter;
  LPF1P::Parameters params;
  params.coefficient_ = 0.5f;

  filter.UpdateParameters(params);

  float output = filter.Process(1.0f);
  EXPECT_TRUE(std::isfinite(output));
}

}  // namespace dsp
}  // namespace soir
