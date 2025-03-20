#include <gtest/gtest.h>

#include "core/engine/dsp.hh"
#include "core/engine/lfo.hh"

namespace soir {
namespace core {
namespace test {

TEST(Lfo, Saw) {
  engine::LFO::Settings settings;

  settings.type_ = engine::LFO::SAW;
  settings.frequency_ = 1.0f;

  engine::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < engine::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, Tri) {
  engine::LFO::Settings settings;

  settings.type_ = engine::LFO::TRI;
  settings.frequency_ = 1.0f;

  engine::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < engine::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, SINE) {
  engine::LFO::Settings settings;

  settings.type_ = engine::LFO::SINE;
  settings.frequency_ = 1.0f;

  engine::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  float previous = lfo.Render();
  for (int i = 0; i < engine::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

}  // namespace test
}  // namespace core
}  // namespace soir
