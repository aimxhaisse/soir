#include <absl/log/log.h>

#include "midi.hh"

namespace maethstro {

Midi::Midi() {}

Midi::~Midi() {}

absl::Status Midi::Init(const Config& config) {
  settings_.grpc_host = config.Get<std::string>("midi.grpc.host");
  settings_.grpc_port = config.Get<int>("midi.grpc.port");

  engine_ = std::make_unique<Engine>();
  auto status = engine_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize engine: " << status;
    return status;
  }

  LOG(INFO) << "Initializing Midi with GPRC host " << settings_.grpc_host
            << " and port " << settings_.grpc_port;

  grpc::ServerBuilder builder;

  builder.AddListeningPort(
      settings_.grpc_host + ":" + std::to_string(settings_.grpc_port),
      grpc::InsecureServerCredentials());
  builder.RegisterService(this);

  grpc_ = builder.BuildAndStart();
  if (!grpc_) {
    return absl::InternalError("Unable to start grpc server");
  }

  LOG(INFO) << "Midi listening on " << settings_.grpc_host << ":"
            << settings_.grpc_port;

  return absl::OkStatus();
}

absl::Status Midi::Run() {
  LOG(INFO) << "Midi running";

  engine_->Run().IgnoreError();
  grpc_->Wait();

  return absl::OkStatus();
}

absl::Status Midi::Wait() {
  grpc_->Wait();
  engine_->Wait().IgnoreError();

  LOG(INFO) << "Midi properly shut down";

  return absl::OkStatus();
}

absl::Status Midi::Stop() {
  LOG(INFO) << "Midi shutting down";

  engine_->Stop().IgnoreError();
  grpc_->Shutdown();

  return absl::OkStatus();
}

grpc::Status Midi::LiveUpdate(grpc::ServerContext* context,
                              const proto::MatinLiveUpdate_Request* request,
                              proto::MatinLiveUpdate_Response* response) {
  if (!request->has_username()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Missing username");
  }
  if (!request->has_code()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Missing code");
  }

  LOG(INFO) << "Midi received live update request from " << request->username()
            << "@" << context->peer();

  auto status = engine_->UpdateCode(request->code());
  if (!status.ok()) {
    LOG(ERROR) << "Unable to update live code: " << status;
    return grpc::Status(grpc::StatusCode::INTERNAL, "Unable to evaluate code");
  }

  return grpc::Status::OK;
}

}  // namespace maethstro
