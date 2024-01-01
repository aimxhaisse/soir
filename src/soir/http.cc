#include <absl/log/log.h>

#include "http.hh"

namespace maethstro {
namespace soir {

HttpServer::HttpServer() {}

HttpServer::~HttpServer() {}

absl::Status HttpServer::Init(const common::Config& config) {
  LOG(INFO) << "Initializing HTTP server";

  http_host_ = config.Get<std::string>("soir.http.host");
  http_port_ = config.Get<int>("soir.http.port");

  server_ = std::make_unique<httplib::Server>();

  return absl::OkStatus();
}

absl::Status HttpServer::Start() {
  LOG(INFO) << "Starting HTTP server";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "HTTP server failed: " << status;
    }
  });

  return absl::OkStatus();
}

absl::Status HttpServer::Run() {
  server_->listen(http_host_.c_str(), http_port_);

  return absl::OkStatus();
}

absl::Status HttpServer::Stop() {
  LOG(INFO) << "Stopping HTTP server";

  server_->stop();
  thread_.join();

  LOG(INFO) << "HTTP server stopped";

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
