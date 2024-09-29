#include <thread>

#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, BasicLoop) {
  PushCode(R"(
bpm.set(600)

i = 0

@loop(track=1, beats=1)
def kick():
  global i
  log('loop ' + str(i))
  i += 1

  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("loop 0"));
  EXPECT_TRUE(WaitForNotification("loop 1"));
  EXPECT_TRUE(WaitForNotification("loop 2"));
  EXPECT_TRUE(WaitForNotification("loop 3"));
  EXPECT_TRUE(WaitForNotification("loop 4"));
  EXPECT_TRUE(WaitForNotification("loop 5"));
  EXPECT_TRUE(WaitForNotification("loop 6"));
  EXPECT_TRUE(WaitForNotification("loop 7"));
  EXPECT_TRUE(WaitForNotification("loop 8"));
  EXPECT_TRUE(WaitForNotification("loop 9"));
}

TEST_F(CoreTestBase, BasicLoopFailedUpdate) {
  PushCode(R"(
bpm.set(600)

i = 0

@loop(track=1, beats=1)
def kick():
  global i

  if i == 0: 
    log('Hello')
    i += 1

  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_TRUE(WaitForNotification("Hello"));

  PushCode(R"(
@loop(track=1, beats=1)
def kick():
  error

  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  PushCode(R"(
@loop(track=1, beats=1)
def kick():
  log('Hello 2')

  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("Hello 2"));
}

}  // namespace test
}  // namespace core
}  // namespace neon
