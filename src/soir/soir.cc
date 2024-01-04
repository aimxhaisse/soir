#include <absl/log/log.h>

#include "soir.hh"

namespace maethstro {
namespace soir {

Soir::Soir() {}

Soir::~Soir() {}

absl::Status Soir::Init(const common::Config& config) {
  LOG(INFO) << "Initializing Soir";

  engine_ = std::make_unique<Engine>();

  auto status = engine_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize engine: " << status;
    return status;
  }

  http_server_ = std::make_unique<HttpServer>();

  status = http_server_->Init(config, engine_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize HTTP server: " << status;
    return status;
  }

  auto grpc_host = config.Get<std::string>("soir.grpc.host");
  auto grpc_port = config.Get<int>("soir.grpc.port");

  LOG(INFO) << "Initializing Soir with GPRC host " << grpc_host << " and port "
            << grpc_port;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(grpc_host + ":" + std::to_string(grpc_port),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(this);

  grpc_ = builder.BuildAndStart();
  if (!grpc_) {
    return absl::InternalError("Unable to start grpc server");
  }

  LOG(INFO) << "Soir listening on " << grpc_host << ":" << grpc_port;

  return absl::OkStatus();
}

absl::Status Soir::Start() {
  LOG(INFO) << "Starting Soir";

  auto status = engine_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start engine: " << status;
    return status;
  }

  status = http_server_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start HTTP server: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Soir::Stop() {
  LOG(INFO) << "Stopping Soir";

  grpc_->Shutdown();
  grpc_->Wait();

  auto status = http_server_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop HTTP server: " << status;
    return status;
  }

  status = engine_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop engine: " << status;
    return status;
  }

  return absl::OkStatus();
}

grpc::Status Soir::MidiEvents(
    ::grpc::ServerContext* context,
    ::grpc::ServerReader<::proto::MidiEvents_Request>* reader,
    ::proto::MidiEvents_Response* response) {

  for (proto::MidiEvents_Request request; reader->Read(&request);) {
    engine_->PushMidiEvent(request);
  }

  return grpc::Status::OK;
}

}  // namespace soir
}  // namespace maethstro
