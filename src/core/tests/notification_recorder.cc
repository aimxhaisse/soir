#include <absl/log/log.h>
#include <absl/strings/str_split.h>
#include <grpc++/grpc++.h>

#include "notification_recorder.hh"

namespace neon {
namespace core {
namespace test {

NotificationRecorder::NotificationRecorder() {}

NotificationRecorder::~NotificationRecorder() {}

absl::Status NotificationRecorder::Init(const utils::Config& config) {
  auto neon_grpc_host = config.Get<std::string>("recorder.neon.grpc.host");
  auto neon_grpc_port = config.Get<int>("recorder.neon.grpc.port");

  neon_stub_ = proto::Neon::NewStub(
      grpc::CreateChannel(neon_grpc_host + ":" + std::to_string(neon_grpc_port),
                          grpc::InsecureChannelCredentials()));
  if (!neon_stub_) {
    return absl::InternalError("Failed to create Neon gRPC stub");
  }

  LOG(INFO) << "Recorder initialized with settings: " << neon_grpc_host << ", "
            << neon_grpc_port;

  return absl::OkStatus();
}

absl::Status NotificationRecorder::Start() {
  LOG(INFO) << "Subscriber starting";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Subscriber failed: " << status.message();
    }
  });

  return absl::OkStatus();
}

absl::Status NotificationRecorder::Stop() {
  LOG(INFO) << "Subscriber shutting down";

  context_.TryCancel();

  if (thread_.joinable()) {
    thread_.join();
  }

  LOG(INFO) << "Subscriber properly shut down";

  notifications_.clear();

  return absl::OkStatus();
}

absl::Status NotificationRecorder::Run() {
  proto::GetLogsRequest request;

  auto stream = neon_stub_->GetLogs(&context_, request);
  if (!stream) {
    return absl::InternalError("Failed to subscribe to log notifications");
  }

  proto::GetLogsResponse response;
  while (stream->Read(&response)) {
    auto entries = absl::StrSplit(response.notification(), '\n');

    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto& entry : entries) {
        notifications_.push_back(std::string(entry));
      }
    }
  }

  return absl::OkStatus();
}

std::vector<std::string> NotificationRecorder::PopNotifications() {
  std::lock_guard<std::mutex> lock(mutex_);
  auto notifications = std::move(notifications_);
  notifications_.clear();
  return notifications;
}

}  // namespace test
}  // namespace core
}  // namespace neon
