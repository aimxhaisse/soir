#include <gtest/gtest.h>

#include "core/dsp.hh"
#include "core/lfo.hh"

namespace soir {
namespace core {
namespace test {

TEST(Lfo, Saw) {
  LFO::Settings settings;

  settings.type_ = LFO::SAW;
  settings.frequency_ = 1.0f;

  LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, Tri) {
  LFO::Settings settings;

  settings.type_ = LFO::TRI;
  settings.frequency_ = 1.0f;

  LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, SINE) {
  LFO::Settings settings;

  settings.type_ = LFO::SINE;
  settings.frequency_ = 1.0f;

  LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  float previous = lfo.Render();
  for (int i = 0; i < kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

}  // namespace test
}  // namespace core
}  // namespace soir
