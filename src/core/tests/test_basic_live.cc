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

}  // namespace test
}  // namespace core
}  // namespace neon
