#include <gtest/gtest.h>

#include "config.hh"

namespace maethstro {
namespace common {
namespace {

class ConfigTest : public testing::Test {};

TEST_F(ConfigTest, EmptyConfig) {
  absl::StatusOr<std::unique_ptr<Config>> config = Config::LoadFromString("");

  EXPECT_TRUE(config.ok());
}

TEST_F(ConfigTest, Simple) {
  absl::StatusOr<std::unique_ptr<Config>> config_or = Config::LoadFromString(R"(

# A comment.
settings:
  a_number: 42
  a_string: 'there is no spoon'
  a_struct:
    another_number: 21
    a_bool: true

  )");

  EXPECT_TRUE(config_or.ok());

  std::unique_ptr<Config> config = std::move(*config_or);

  EXPECT_EQ(config->Get<int>("settings.a_number"), 42);
  EXPECT_EQ(config->Get<std::string>("settings.a_string"), "there is no spoon");
  EXPECT_EQ(config->Get<int>("settings.a_struct.another_number"), 21);
  EXPECT_EQ(config->Get<bool>("settings.a_struct.a_bool"), true);
}

}  // namespace
}  // namespace common
}  // namespace maethstro
