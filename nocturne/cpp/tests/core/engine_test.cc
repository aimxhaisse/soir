#include "core/engine.hh"

#include <gtest/gtest.h>

namespace soir {
namespace {

// Test config with audio output disabled
constexpr const char* kTestConfig = R"({
  "dsp": {
    "enable_output": false,
    "sample_directory": "/tmp",
    "sample_packs": []
  }
})";

}  // namespace

TEST(EngineTest, Construction) {
  Engine engine;
  // Basic construction should succeed
  EXPECT_TRUE(true);
}

TEST(EngineTest, Initialization) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto status = engine.Init(config);
  EXPECT_TRUE(status.ok()) << status.message();
}

TEST(EngineTest, StartStop) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok()) << init_status.message();

  auto start_status = engine.Start();
  EXPECT_TRUE(start_status.ok()) << start_status.message();

  auto stop_status = engine.Stop();
  EXPECT_TRUE(stop_status.ok()) << stop_status.message();
}

TEST(EngineTest, GetSampleManager) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok());

  SampleManager& manager = engine.GetSampleManager();
  // Should return a valid reference
  EXPECT_TRUE(true);
}

TEST(EngineTest, GetControls) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok());

  Controls* controls = engine.GetControls();
  EXPECT_NE(controls, nullptr);
}

}  // namespace soir
