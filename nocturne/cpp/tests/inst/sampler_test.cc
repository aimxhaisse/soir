#include "inst/sampler.hh"

#include <gtest/gtest.h>

#include "core/controls.hh"
#include "core/sample_manager.hh"

namespace soir {
namespace inst {

TEST(SamplerTest, Creation) {
  Sampler sampler;
  EXPECT_TRUE(true);
}

TEST(SamplerTest, Initialization) {
  Sampler sampler;
  SampleManager sample_manager;
  Controls controls;

  auto status = sampler.Init("", &sample_manager, &controls);
  EXPECT_TRUE(status.ok());
}

TEST(SamplerTest, GetType) {
  Sampler sampler;
  EXPECT_EQ(sampler.GetType(), Type::SAMPLER);
}

}  // namespace inst
}  // namespace soir
