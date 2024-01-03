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

  LOG(INFO) << "Soir initialized";

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

}  // namespace soir
}  // namespace maethstro
