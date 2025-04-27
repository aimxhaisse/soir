#include <absl/log/log.h>
#include <absl/strings/str_split.h>

#include "subscriber.hh"

namespace soir {
namespace agent {

Subscriber::Subscriber() {}

Subscriber::~Subscriber() {}

absl::Status Subscriber::Init(const utils::Config& config) {
  soir_grpc_host_ = config.Get<std::string>("agent.soir.grpc.host");
  soir_grpc_port_ = config.Get<int>("agent.soir.grpc.port");

  soir_stub_ = proto::Soir::NewStub(grpc::CreateChannel(
      soir_grpc_host_ + ":" + std::to_string(soir_grpc_port_),
      grpc::InsecureChannelCredentials()));
  if (!soir_stub_) {
    return absl::InternalError("Failed to create Soir gRPC stub");
  }

  LOG(INFO) << "Subscriber initialized with settings: " << soir_grpc_host_
            << ", " << soir_grpc_port_;

  return absl::OkStatus();
}

absl::Status Subscriber::Start() {
  LOG(INFO) << "Subscriber starting";

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

  auto stream = soir_stub_->GetLogs(&context_, request);
  if (!stream) {
    return absl::InternalError("Failed to subscribe to log notifications");
  }

  proto::GetLogsResponse response;
  while (stream->Read(&response)) {
    auto entries = absl::StrSplit(response.notification(), '\n');

    for (const auto& entry : entries) {
      LOG(INFO) << "\e[1;42mS O I R \e[0m\e[1;32m "
                << ">\e[0m " << entry;

      std::cout << "\e[1;43mS O I R \e[0m\e[1;33m "
                << ">\e[0m " << entry << std::endl;
      std::cout.flush();
    }
  }

  return absl::OkStatus();
}

}  // namespace agent
}  // namespace soir
