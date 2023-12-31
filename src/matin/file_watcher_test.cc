#include <gtest/gtest.h>

#include "file_watcher.hh"

namespace maethstro {
namespace matin {
namespace {

class FileWatcherTest : public testing::Test {};

TEST_F(FileWatcherTest, IsLiveCodingFile) {
  FileWatcher watcher;

  EXPECT_TRUE(watcher.IsLiveCodingFile("live.py"));
  EXPECT_TRUE(watcher.IsLiveCodingFile("01_live.py"));

  EXPECT_FALSE(watcher.IsLiveCodingFile("live.py42"));
  EXPECT_FALSE(watcher.IsLiveCodingFile(".py42"));
  EXPECT_FALSE(watcher.IsLiveCodingFile(".live.py"));
  EXPECT_FALSE(watcher.IsLiveCodingFile("#live.py#"));
}

}  // namespace
}  // namespace matin
}  // namespace maethstro
