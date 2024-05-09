#include "utils/signal.hh"

#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, TestExit) {
  PushCode(R"(
raise SystemExit();
  )");

  EXPECT_TRUE(utils::WaitForExitSignal().ok());
}

}  // namespace test
}  // namespace core
}  // namespace neon
