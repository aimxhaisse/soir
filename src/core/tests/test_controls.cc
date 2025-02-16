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

TEST_F(CoreTestBase, ControlsGlobal) {
  PushCode(R"(
ctrls.mk_lfo("c1", 0.5)
log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));
}

TEST_F(CoreTestBase, ControlsGlobalDeletion) {
  PushCode(R"(
ctrls.mk_lfo("c1", 0.5)
log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));

  PushCode("log('empty-eval')");

  EXPECT_TRUE(WaitForNotification("empty-eval"));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, ControlsDeletionInLive) {
  PushCode(R"(
@live()
def setup():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));

  PushCode(R"(
@live()
def setup():
    log('empty-eval')
  )");

  EXPECT_TRUE(WaitForNotification("empty-eval"));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
@live()
def setup():
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, ControlsLiveFunctionDeleted) {
  PushCode(R"(
@live()
def setup():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));

  PushCode(R"(
log('empty-eval')
  )");

  EXPECT_TRUE(WaitForNotification("empty-eval"));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, ControlsDeletionInLoop) {
  PushCode(R"(
@loop(beats=1)
def helloop():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));

  PushCode(R"(
@live()
@loop(beats=1)
def helloop():
    log('empty-eval')
  )");

  EXPECT_TRUE(WaitForNotification("empty-eval"));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
@live()
@loop(beats=1)
def helloop():
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, ControlsLoopFunctionDeleted) {
  PushCode(R"(
@loop(beats=1)
def helloop():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[[c1=0]]"));
  PushCode(R"(
log('empty-eval')
  )");

  EXPECT_TRUE(WaitForNotification("empty-eval"));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
log(str(ctrls.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
