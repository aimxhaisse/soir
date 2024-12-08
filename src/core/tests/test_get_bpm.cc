#include "base.hh"

namespace soir {
namespace core {
namespace test {

TEST_F(CoreTestBase, GetBpmOK) {
  PushCode(R"(
bpm.set(120);
log(str(bpm.get()));
bpm.set(60);
log(str(bpm.get()));
  )");

  EXPECT_TRUE(WaitForNotification("120.0"));
  EXPECT_TRUE(WaitForNotification("60.0"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
