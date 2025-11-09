#include "dsp/tools.hh"

#include <gtest/gtest.h>

TEST(DspToolsTest, UnipolarConversion) {
  // Test bipolar to unipolar conversion
  EXPECT_FLOAT_EQ(soir::dsp::Unipolar(-1.0f), 0.0f);
  EXPECT_FLOAT_EQ(soir::dsp::Unipolar(0.0f), 0.5f);
  EXPECT_FLOAT_EQ(soir::dsp::Unipolar(1.0f), 1.0f);
}

TEST(DspToolsTest, BipolarConversion) {
  // Test unipolar to bipolar conversion
  EXPECT_FLOAT_EQ(soir::dsp::Bipolar(0.0f), -1.0f);
  EXPECT_FLOAT_EQ(soir::dsp::Bipolar(0.5f), 0.0f);
  EXPECT_FLOAT_EQ(soir::dsp::Bipolar(1.0f), 1.0f);
}

TEST(DspToolsTest, RoundTripConversion) {
  // Test that converting back and forth gives the original value
  float bipolar_value = 0.75f;
  float unipolar_value = 0.8f;

  EXPECT_FLOAT_EQ(soir::dsp::Bipolar(soir::dsp::Unipolar(bipolar_value)),
                  bipolar_value);
  EXPECT_FLOAT_EQ(soir::dsp::Unipolar(soir::dsp::Bipolar(unipolar_value)),
                  unipolar_value);
}
