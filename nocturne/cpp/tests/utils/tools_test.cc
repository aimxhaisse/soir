#include "utils/tools.hh"

#include <gtest/gtest.h>

namespace soir {

TEST(ToolsTest, PanningLeft) {
  EXPECT_FLOAT_EQ(LeftPan(0.0f), 1.0f);
  EXPECT_FLOAT_EQ(LeftPan(1.0f), 0.0f);
  EXPECT_FLOAT_EQ(LeftPan(-1.0f), 1.0f);
}

TEST(ToolsTest, PanningRight) {
  EXPECT_FLOAT_EQ(RightPan(0.0f), 1.0f);
  EXPECT_FLOAT_EQ(RightPan(-1.0f), 0.0f);
  EXPECT_FLOAT_EQ(RightPan(1.0f), 1.0f);
}

TEST(ToolsTest, BipolarUnipolarConversion) {
  EXPECT_FLOAT_EQ(Bipolar(0.5f), 0.0f);
  EXPECT_FLOAT_EQ(Bipolar(1.0f), 1.0f);
  EXPECT_FLOAT_EQ(Bipolar(0.0f), -1.0f);

  EXPECT_FLOAT_EQ(Unipolar(0.0f), 0.5f);
  EXPECT_FLOAT_EQ(Unipolar(1.0f), 1.0f);
  EXPECT_FLOAT_EQ(Unipolar(-1.0f), 0.0f);
}

TEST(ToolsTest, Clip) {
  EXPECT_FLOAT_EQ(Clip(0.5f, 0.0f, 1.0f), 0.5f);
  EXPECT_FLOAT_EQ(Clip(-0.5f, 0.0f, 1.0f), 0.0f);
  EXPECT_FLOAT_EQ(Clip(1.5f, 0.0f, 1.0f), 1.0f);
}

TEST(ToolsTest, Fabs) {
  EXPECT_FLOAT_EQ(Fabs(1.0f), 1.0f);
  EXPECT_FLOAT_EQ(Fabs(-1.0f), 1.0f);
  EXPECT_FLOAT_EQ(Fabs(0.0f), 0.0f);
}

}  // namespace soir
