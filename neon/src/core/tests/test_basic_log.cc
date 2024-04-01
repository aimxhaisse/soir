#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, BasicLogOK) {
  PushCode(R"(
log('hello, world');
  )");

  EXPECT_TRUE(WaitForNotification("hello, world"));
}

TEST_F(CoreTestBase, BasicLogKO) {
  PushCode(R"(
log('hello, world');
  )");

  EXPECT_FALSE(WaitForNotification("woot"));
}

}  // namespace test
}  // namespace core
}  // namespace neon
