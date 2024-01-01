#include <absl/log/log.h>

#include "http.hh"

namespace maethstro {
namespace soir {

HttpServer::HttpServer() {}

HttpServer::~HttpServer() {}

absl::Status HttpServer::Init(const common::Config& config) {
  LOG(INFO) << "Initializing HTTP server";
  return absl::OkStatus();
}

absl::Status HttpServer::Start() {
  LOG(INFO) << "Starting HTTP server";
  return absl::OkStatus();
}

absl::Status HttpServer::Stop() {
  LOG(INFO) << "Stopping HTTP server";
  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
