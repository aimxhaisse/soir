#include <gtest/gtest.h>

#include "matin.hh"

namespace maethstro {
namespace matin {
namespace {

class MatinTest : public testing::Test {};

TEST_F(MatinTest, IsLiveCodingFile) {
  Matin matin;

  EXPECT_TRUE(matin.IsLiveCodingFile("live.py"));
  EXPECT_TRUE(matin.IsLiveCodingFile("01_live.py"));

  EXPECT_FALSE(matin.IsLiveCodingFile("live.py42"));
  EXPECT_FALSE(matin.IsLiveCodingFile(".py42"));
  EXPECT_FALSE(matin.IsLiveCodingFile(".live.py"));
  EXPECT_FALSE(matin.IsLiveCodingFile("#live.py#"));
}

}  // namespace
}  // namespace matin
}  // namespace maethstro
