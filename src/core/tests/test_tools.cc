#include <gtest/gtest.h>

#include "core/tools.hh"

namespace soir {
namespace core {
namespace test {

TEST(Tools, LeftPan) {
  ASSERT_EQ(LeftPan(-1.0f), 1.0f);
  ASSERT_EQ(LeftPan(-0.5f), 1.0f);
  ASSERT_EQ(LeftPan(-0.3f), 1.0f);
  ASSERT_EQ(LeftPan(0.0f), 1.0f);
  ASSERT_EQ(LeftPan(0.3f), 0.7f);
  ASSERT_EQ(LeftPan(0.5f), 0.5f);
  ASSERT_EQ(LeftPan(1.0f), 0.0f);
}

TEST(Tools, RightPan) {
  ASSERT_EQ(RightPan(-1.0f), 0.0f);
  ASSERT_EQ(RightPan(-0.5f), 0.5f);
  ASSERT_EQ(RightPan(-0.3f), 0.7f);
  ASSERT_EQ(RightPan(0.0f), 1.0f);
  ASSERT_EQ(RightPan(0.3f), 1.0f);
  ASSERT_EQ(RightPan(0.5f), 1.0f);
  ASSERT_EQ(RightPan(1.0f), 1.0f);
}

}  // namespace test
}  // namespace core
}  // namespace soir
