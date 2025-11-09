#include <gtest/gtest.h>

#include "core/sample.hh"

namespace soir {

TEST(SampleTest, DurationCalculation) {
  Sample sample;
  sample.lb_.resize(48000);
  sample.rb_.resize(48000);

  EXPECT_EQ(sample.DurationSamples(), 48000);
  EXPECT_FLOAT_EQ(sample.DurationMs(), 1000.0f);
  EXPECT_FLOAT_EQ(sample.DurationMs(24000), 500.0f);
}

}  // namespace soir
