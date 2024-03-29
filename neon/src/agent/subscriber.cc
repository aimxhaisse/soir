#include <absl/log/log.h>
#include <absl/strings/str_split.h>

#include "subscriber.hh"

namespace neon {
namespace agent {

Subscriber::Subscriber() {}

Subscriber::~Subscriber() {}

absl::Status Subscriber::Init(const utils::Config& config) {
  neon_grpc_host_ = config.Get<std::string>("agent.neon.grpc.host");
  neon_grpc_port_ = config.Get<int>("agent.neon.grpc.port");

  neon_stub_ = proto::Neon::NewStub(grpc::CreateChannel(
      neon_grpc_host_ + ":" + std::to_string(neon_grpc_port_),
      grpc::InsecureChannelCredentials()));
  if (!neon_stub_) {
    return absl::InternalError("Failed to create Neon gRPC stub");
  }

  LOG(INFO) << "Subscriber initialized with settings: " << neon_grpc_host_
            << ", " << neon_grpc_port_;

  return absl::OkStatus();
}

absl::Status Subscriber::Start() {
  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Subscriber failed: " << status.message();
    }
  });

  return absl::OkStatus();
}

absl::Status Subscriber::Stop() {
  LOG(INFO) << "Subscriber shutting down";

  context_.TryCancel();

  if (thread_.joinable()) {
    thread_.join();
  }

  LOG(INFO) << "Subscriber properly shut down";

  return absl::OkStatus();
}

absl::Status Subscriber::Run() {
  proto::GetLogsRequest request;

  auto stream = neon_stub_->GetLogs(&context_, request);
  if (!stream) {
    return absl::InternalError("Failed to subscribe to log notifications");
  }

  proto::GetLogsResponse response;
  while (stream->Read(&response)) {
    auto entries = absl::StrSplit(response.notification(), '\n');

    for (const auto& entry : entries) {
      LOG(INFO) << "\e[1;42mN E O N \e[0m\e[1;32m "
                << ">\e[0m " << entry;
    }
  }

  return absl::OkStatus();
}

}  // namespace agent
}  // namespace neon
