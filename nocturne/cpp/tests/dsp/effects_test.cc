#include "dsp/chorus.hh"
#include "dsp/reverb.hh"

#include <gtest/gtest.h>

namespace soir {
namespace dsp {

TEST(ChorusTest, Construction) {
  Chorus chorus;
  EXPECT_TRUE(true);
}

TEST(ChorusTest, ProcessStereo) {
  Chorus chorus;
  Chorus::Parameters params;
  params.time_ = 0.5f;
  params.depth_ = 0.5f;
  params.rate_ = 1.0f;

  chorus.Init(params);

  auto result = chorus.Render(0.5f, 0.5f);
  EXPECT_TRUE(std::isfinite(result.first));
  EXPECT_TRUE(std::isfinite(result.second));
}

TEST(ReverbTest, Construction) {
  Reverb reverb;
  EXPECT_TRUE(true);
}

TEST(ReverbTest, ProcessStereo) {
  Reverb reverb;
  Reverb::Parameters params;
  params.time_ = 0.5f;
  params.absorbency_ = 0.2f;

  reverb.Init(params);

  auto result = reverb.Process(0.5f, 0.5f);
  EXPECT_TRUE(std::isfinite(result.first));
  EXPECT_TRUE(std::isfinite(result.second));
}

}  // namespace dsp
}  // namespace soir
