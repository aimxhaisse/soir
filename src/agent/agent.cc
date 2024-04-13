#include <absl/log/log.h>
#include <filesystem>

#include "agent/agent.hh"

namespace neon {
namespace agent {

Agent::Agent() {}

Agent::~Agent() {}

absl::Status Agent::Init(const utils::Config& config) {
  auto neon_grpc_host = config.Get<std::string>("agent.neon.grpc.host");
  auto neon_grpc_port = config.Get<int>("agent.neon.grpc.port");

  neon_stub_ = proto::Neon::NewStub(
      grpc::CreateChannel(neon_grpc_host + ":" + std::to_string(neon_grpc_port),
                          grpc::InsecureChannelCredentials()));
  if (!neon_stub_) {
    return absl::InternalError("Failed to create Neon gRPC stub");
  }

  file_watcher_ = std::make_unique<FileWatcher>();
  auto status = file_watcher_->Init(config, neon_stub_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize file watcher: " << status.message();
    return status;
  }

  controller_watcher_ = std::make_unique<ControllerWatcher>();
  status = controller_watcher_->Init(config, neon_stub_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize controller watcher: "
               << status.message();
    return status;
  }

  subscriber_ = std::make_unique<Subscriber>();
  status = subscriber_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize subscriber: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Agent::Start() {
  auto status = file_watcher_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start file watcher: " << status.message();
    return status;
  }

  status = subscriber_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start subscriber: " << status.message();
    return status;
  }

  status = controller_watcher_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start controller watcher: " << status.message();
    return status;
  }

  LOG(INFO) << "Agent running";

  return absl::OkStatus();
}

absl::Status Agent::Stop() {
  LOG(INFO) << "Agent stopping";

  auto status = controller_watcher_->Stop();
  if (!status.ok()) {
    LOG(WARNING) << "Controller watcher failed to stop: " << status.message();
  }

  status = subscriber_->Stop();
  if (!status.ok()) {
    LOG(WARNING) << "Subscriber failed to stop: " << status.message();
  }

  status = file_watcher_->Stop();
  if (!status.ok()) {
    LOG(WARNING) << "File watcher failed to stop: " << status.message();
  }

  LOG(INFO) << "Agent stopped";

  return absl::OkStatus();
}

}  // namespace agent
}  // namespace neon
