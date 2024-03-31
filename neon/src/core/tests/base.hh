#pragma once

#include <gtest/gtest.h>

namespace neon {
namespace core {
namespace test {

class CoreTestBase : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;
};

}  // namespace test
}  // namespace core
}  // namespace neon
