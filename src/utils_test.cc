#include <gtest/gtest.h>

#include "utils.h"

namespace soir {
namespace {

TEST(StringSplitTest, NoDelim) {
  const std::vector<std::string> s =
      utils::StringSplit("there is no spoon", '.');

  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s[0], "there is no spoon");
}

TEST(StringSplitTest, Empty) {
  const std::vector<std::string> s = utils::StringSplit("", '.');

  EXPECT_EQ(s.size(), 0);
}

TEST(StringSplitTest, Delim) {
  const std::vector<std::string> s =
      utils::StringSplit("there is no spoon", ' ');

  EXPECT_EQ(s.size(), 4);
  EXPECT_EQ(s[0], "there");
  EXPECT_EQ(s[1], "is");
  EXPECT_EQ(s[2], "no");
  EXPECT_EQ(s[3], "spoon");
}

TEST(StringSplitTest, MultipleConsecutiveDelims) {
  const std::vector<std::string> s =
      utils::StringSplit(" there  is      no spoon  ", ' ');

  EXPECT_EQ(s.size(), 4);
  EXPECT_EQ(s[0], "there");
  EXPECT_EQ(s[1], "is");
  EXPECT_EQ(s[2], "no");
  EXPECT_EQ(s[3], "spoon");
}

} // namespace
} // namespace soir
