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
    initial_bpm: 130
  grpc:
    host: localhost
    port: 9000
  dsp:
    http:
      host: localhost
      port: 8081
    engine:
      block_size: 4096
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
  std::vector<std::string> notifications;

  for (int i = 0; i < 10; ++i) {
    notifications = recorder_->PopNotifications();

    for (const std::string& n : notifications) {
      if (n == notification) {
        return true;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return false;
}

}  // namespace test
}  // namespace core
}  // namespace neon
