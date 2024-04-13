#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, GetBpmOK) {
  PushCode(R"(
set_bpm(120);
log(str(get_bpm()));
set_bpm(60);
log(str(get_bpm()));
  )");

  EXPECT_TRUE(WaitForNotification("120.0"));
  EXPECT_TRUE(WaitForNotification("60.0"));
}

}  // namespace test
}  // namespace core
}  // namespace neon
