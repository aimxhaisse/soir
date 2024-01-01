#include <absl/log/log.h>

#include "soir.hh"

namespace maethstro {
namespace soir {

Soir::Soir() {}

Soir::~Soir() {}

absl::Status Soir::Init(const common::Config& config) {
  LOG(INFO) << "Initializing Soir";

  http_server_ = std::make_unique<HttpServer>();

  auto status = http_server_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize HTTP server: " << status;
    return status;
  }

  LOG(INFO) << "Soir initialized";

  return absl::OkStatus();
}

absl::Status Soir::Start() {
  LOG(INFO) << "Starting Soir";
  return absl::OkStatus();
}

absl::Status Soir::Stop() {
  LOG(INFO) << "Stopping Soir";
  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
