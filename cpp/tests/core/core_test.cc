#include <gtest/gtest.h>

#include "core/adsr.hh"
#include "core/midi_event.hh"
#include "core/midi_stack.hh"
#include "core/parameter.hh"

namespace soir {

TEST(ADSRTest, Initialization) {
  ADSR adsr;
  auto status = adsr.Init(100.0f, 200.0f, 150.0f, 0.8f);
  EXPECT_TRUE(status.ok());
}

TEST(MidiStackTest, Creation) {
  MidiStack stack;
  // Basic test to ensure it compiles
  EXPECT_TRUE(true);
}

TEST(ParameterTest, ConstantValue) {
  Parameter p(0.5f);
  EXPECT_FLOAT_EQ(p.GetValue(0), 0.5f);
}

TEST(ParameterTest, WithRange) {
  Parameter p(0.5f, 0.0f, 1.0f);
  p.SetConstant(1.5f);
  // Should be clamped to max
  EXPECT_FLOAT_EQ(p.GetValue(0), 1.0f);
}

}  // namespace soir
