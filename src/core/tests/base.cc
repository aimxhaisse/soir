#include <absl/log/log.h>

#include "base.hh"

namespace neon {
namespace core {
namespace test {

void CoreTestBase::SetUp() {
  absl::StatusOr<std::unique_ptr<utils::Config>> config_or =
      utils::Config::LoadFromString(R"(
recorder:
  neon:
    grpc:
      host: localhost
      port: 9000
neon:
  rt:
    python_path: src/core/rt/py
    initial_bpm: 130
  grpc:
    host: localhost
    port: 9000
  dsp:
    http:
      host: localhost
      port: 8081
  )");

  EXPECT_TRUE(config_or.ok());

  config_ = std::move(*config_or);
  neon_ = std::make_unique<Neon>();
  recorder_ = std::make_unique<NotificationRecorder>();

  absl::Status status = neon_->Init(*config_);
  EXPECT_TRUE(status.ok());

  status = recorder_->Init(*config_);
  EXPECT_TRUE(status.ok());

  status = neon_->Start();
  EXPECT_TRUE(status.ok());

  status = recorder_->Start();
  EXPECT_TRUE(status.ok());
}

void CoreTestBase::TearDown() {
  absl::Status status = neon_->Stop();
  EXPECT_TRUE(status.ok());

  status = recorder_->Stop();
  EXPECT_TRUE(status.ok());

  recorder_.reset();
  config_.reset();
  neon_.reset();
}

void CoreTestBase::PushCode(const std::string& code) {
  proto::PushCodeUpdateRequest request;
  proto::PushCodeUpdateResponse response;
  grpc::ServerContext context;

  request.set_code(code);

  grpc::Status status = neon_->PushCodeUpdate(&context, &request, &response);

  EXPECT_TRUE(status.ok());
}

bool CoreTestBase::WaitForNotification(const std::string& notification) {
  for (int i = 0; i < 10; ++i) {
    while (!notifications_.empty()) {
      auto entry = notifications_.front();

      LOG(INFO) << "\e[1;42mN E O N \e[0m\e[1;32m "
                << ">\e[0m " << entry << "\n";

      notifications_.erase(notifications_.begin());

      if (entry == notification) {
        return true;
      }
    }

    auto new_notifications = recorder_->PopNotifications();

    notifications_.insert(notifications_.end(), new_notifications.begin(),
                          new_notifications.end());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return false;
}

}  // namespace test
}  // namespace core
}  // namespace neon
