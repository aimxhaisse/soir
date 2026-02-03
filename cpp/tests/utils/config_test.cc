#include "utils/config.hh"

#include <gtest/gtest.h>

namespace soir {
namespace utils {
namespace {

class ConfigTest : public testing::Test {};

TEST_F(ConfigTest, EmptyConfig) {
  Config config("{}");
  // Empty config should parse successfully
}

TEST_F(ConfigTest, Simple) {
  Config config(R"({
    "settings": {
      "a_number": 42,
      "a_string": "there is no spoon",
      "a_struct": {
        "another_number": 21,
        "a_bool": true
      }
    }
  })");

  EXPECT_EQ(config.Get<int>("settings.a_number"), 42);
  EXPECT_EQ(config.Get<std::string>("settings.a_string"), "there is no spoon");
  EXPECT_EQ(config.Get<int>("settings.a_struct.another_number"), 21);
  EXPECT_EQ(config.Get<bool>("settings.a_struct.a_bool"), true);
}

TEST_F(ConfigTest, Vectors) {
  Config config(R"({
    "numbers": [1, 2, 3, 4, 5],
    "strings": ["foo", "bar", "baz"],
    "nested": {
      "channels": [0, 1]
    }
  })");

  auto numbers = config.Get<std::vector<int>>("numbers");
  EXPECT_EQ(numbers.size(), 5);
  EXPECT_EQ(numbers[0], 1);
  EXPECT_EQ(numbers[4], 5);

  auto strings = config.Get<std::vector<std::string>>("strings");
  EXPECT_EQ(strings.size(), 3);
  EXPECT_EQ(strings[0], "foo");
  EXPECT_EQ(strings[2], "baz");

  auto channels = config.Get<std::vector<int>>("nested.channels");
  EXPECT_EQ(channels.size(), 2);
  EXPECT_EQ(channels[0], 0);
  EXPECT_EQ(channels[1], 1);
}

TEST_F(ConfigTest, NestedConfig) {
  Config config(R"({
    "audio": {
      "sample_rate": 48000,
      "buffer_size": 512
    }
  })");

  Config audio = config.Get<Config>("audio");
  EXPECT_EQ(audio.Get<int>("sample_rate"), 48000);
  EXPECT_EQ(audio.Get<int>("buffer_size"), 512);
}

}  // namespace
}  // namespace utils
}  // namespace soir
