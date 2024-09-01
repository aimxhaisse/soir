#include <thread>

#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, BasicLive) {
  PushCode(R"(
@live()
def kick():
  log('hello')
  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("hello"));
}

TEST_F(CoreTestBase, BasicLiveNoUpdate) {
  PushCode(R"(
@live()
def kick():
  log('hello')
  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("hello"));

  PushCode(R"(
@live()
def kick():
  log('hello')
  )");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_FALSE(WaitForNotification("hello"));
}

TEST_F(CoreTestBase, BasicLiveGetCode) {
  PushCode(R"(
@live()
def kick():
  log('hello')
  pass
  log('world')

log(str(neon.internals.get_live('kick').code))
)");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("  log('hello')"));
  EXPECT_TRUE(WaitForNotification("  pass"));
  EXPECT_TRUE(WaitForNotification("  log('world')"));

  PushCode(R"(
@live()
def kick():
  log('hello')
  pass
  log('world')
  x = 10
  if x == 42:
    log('42')
  else:
    log('not 42')

log(str(neon.internals.get_live('kick').code))
)");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForNotification("  log('hello')"));
  EXPECT_TRUE(WaitForNotification("  pass"));
  EXPECT_TRUE(WaitForNotification("  log('world')"));
  EXPECT_TRUE(WaitForNotification("  x = 10"));
  EXPECT_TRUE(WaitForNotification("  if x == 42:"));
  EXPECT_TRUE(WaitForNotification("    log('42')"));
  EXPECT_TRUE(WaitForNotification("  else:"));
  EXPECT_TRUE(WaitForNotification("    log('not 42')"));
}

}  // namespace test
}  // namespace core
}  // namespace neon
