#include <thread>

#include "base.hh"

namespace soir {
namespace core {
namespace test {

TEST_F(CoreTestBase, ControlsBasic) {
  PushCode(R"(
@live()
def controls():
    ctrls.mk_lfo("c1", 0.5)
    ctrls.mk_linear("c2", 0.5, 2.0, 8.0)


log(str(ctrls.layout()))
  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("[[c1=0], [c2=0.5]]"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
