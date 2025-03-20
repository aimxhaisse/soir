#include <gtest/gtest.h>
#include <pybind11/pybind11.h>

#include "core/engine/controls.hh"
#include "core/engine/dsp.hh"
#include "core/engine/lfo.hh"
#include "core/parameter.hh"

namespace py = pybind11;

namespace soir {
namespace core {
namespace test {

TEST(Parameter, TestConstant) {
  Parameter p(1.0f);
  EXPECT_EQ(p.GetValue(0), 1.0f);
  EXPECT_EQ(p.GetValue(100), 1.0f);
  EXPECT_EQ(std::get<float>(p.Raw()), 1.0f);
}

}  // namespace test
}  // namespace core
}  // namespace soir
