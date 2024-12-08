#pragma once

#include <gtest/gtest.h>

#include "core/soir.hh"
#include "notification_recorder.hh"
#include "utils/config.hh"

namespace soir {
namespace core {
namespace test {

class CoreTestBase : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

  void PushCode(const std::string& code);
  bool WaitForNotification(const std::string& notification);

 private:
  std::unique_ptr<utils::Config> config_;
  std::unique_ptr<Soir> soir_;
  std::unique_ptr<NotificationRecorder> recorder_;
  std::vector<std::string> notifications_;
};

}  // namespace test
}  // namespace core
}  // namespace soir
