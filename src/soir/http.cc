#include <absl/log/log.h>

#include "http.hh"

namespace maethstro {
namespace soir {

HttpStream::HttpStream() {}

HttpStream::~HttpStream() {}

absl::Status HttpStream::PushSamples(const std::string& data) {
  std::unique_lock<std::mutex> lock(mutex_);
  stream_ = data;
  cond_.notify_one();
  return absl::OkStatus();
}

absl::Status HttpStream::Run(httplib::Response& response) {
  LOG(INFO) << "Starting HTTP stream";

  while (true) {
    std::string stream;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cond_.wait(lock);
      if (!running_) {
        break;
      }
      std::swap(stream, stream_);
    }

    response.set_content(stream, "application/ogg");
  }

  return absl::OkStatus();
}

absl::Status HttpStream::Stop() {
  LOG(INFO) << "Stopping HTTP stream";

  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  cond_.notify_all();

  LOG(INFO) << "HTTP stream stopped";

  return absl::OkStatus();
}

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

  server_->Get(
      "/", [&](const httplib::Request& request, httplib::Response& response) {
        //@
      });

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
