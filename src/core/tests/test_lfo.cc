#include <gtest/gtest.h>

#include "core/dsp/dsp.hh"
#include "core/dsp/lfo.hh"

namespace soir {
namespace core {
namespace test {

TEST(Lfo, Saw) {
  dsp::LFO::Settings settings;

  settings.type_ = dsp::LFO::SAW;
  settings.frequency_ = 1.0f;

  dsp::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < dsp::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, Tri) {
  dsp::LFO::Settings settings;

  settings.type_ = dsp::LFO::TRI;
  settings.frequency_ = 1.0f;

  dsp::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  for (int i = 0; i < dsp::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

TEST(Lfo, SINE) {
  dsp::LFO::Settings settings;

  settings.type_ = dsp::LFO::SINE;
  settings.frequency_ = 1.0f;

  dsp::LFO lfo;

  ASSERT_TRUE(lfo.Init(settings).ok());

  float previous = lfo.Render();
  for (int i = 0; i < dsp::kSampleRate / 10; i++) {
    float value = lfo.Render();
    EXPECT_GE(value, -1.0f);
    EXPECT_LE(value, 1.0f);
  }
}

}  // namespace test
}  // namespace core
}  // namespace soir
