#include <gtest/gtest.h>
#include <cstdlib>

#include "utils/config.hh"

namespace soir {
namespace utils {
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

TEST_F(ConfigTest, EnvironmentVariableExpansion) {
  // Set environment variables for testing
  ::setenv("SOIR_TEST_VAR", "test_value", 1);
  ::setenv("SOIR_HOME_DIR", "/home/user", 1);
  ::setenv("SOIR_PORT", "8080", 1);

  absl::StatusOr<std::unique_ptr<Config>> config_or = Config::LoadFromString(R"(
env_vars:
  simple: '$SOIR_TEST_VAR'
  path: '$SOIR_HOME_DIR/music'
  mixed: 'Server running on port $SOIR_PORT'
  missing: '$NONEXISTENT_VAR'
  partial: 'prefix_$SOIR_TEST_VAR_suffix'
  )");

  EXPECT_TRUE(config_or.ok());
  std::unique_ptr<Config> config = std::move(*config_or);

  // Test simple variable expansion
  EXPECT_EQ(config->Get<std::string>("env_vars.simple"), "test_value");

  // Test variable within a path
  EXPECT_EQ(config->Get<std::string>("env_vars.path"), "/home/user/music");

  // Test variable within text
  EXPECT_EQ(config->Get<std::string>("env_vars.mixed"),
            "Server running on port 8080");

  // Test non-existent variable (should remain as is)
  EXPECT_EQ(config->Get<std::string>("env_vars.missing"), "$NONEXISTENT_VAR");

  // Test partial variable match (environment variable doesn't exist, but similar name exists)
  EXPECT_EQ(config->Get<std::string>("env_vars.partial"),
            "prefix_$SOIR_TEST_VAR_suffix");

  // Clean up environment variables
  ::unsetenv("SOIR_TEST_VAR");
  ::unsetenv("SOIR_HOME_DIR");
  ::unsetenv("SOIR_PORT");
}

}  // namespace
}  // namespace utils
}  // namespace soir
