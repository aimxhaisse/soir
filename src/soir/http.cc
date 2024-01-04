#include <absl/log/log.h>

#include "http.hh"

namespace maethstro {
namespace soir {

HttpServer::HttpServer() {}

HttpServer::~HttpServer() {}

absl::Status HttpServer::Init(const common::Config& config, Engine* engine) {
  LOG(INFO) << "Initializing HTTP server";

  engine_ = engine;

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
  server_->Get("/", [this](const httplib::Request& request,
                           httplib::Response& response) {
    LOG(INFO) << "New HTTP stream request";

    // All of this is a bit confusing, but it seems like httplib is
    // not meant to be used simply in a streaming mode so there is a
    // lot of weird back and forth going on here.
    auto stream = new HttpStream();

    // Register the stream to receive audio buffers
    engine_->RegisterConsumer(stream);

    response.set_content_provider(
        "application/octet-stream",
        [this, stream](size_t, httplib::DataSink& sink) {
          auto status = stream->Encode(sink);
          if (!status.ok()) {
            LOG(INFO) << "HTTP stream cancelled because " << status;
            sink.done();
            return false;
          }

          return true;
        },
        [this, stream](bool) {
          LOG(INFO) << "HTTP stream clean up";
          engine_->RemoveConsumer(stream);
          delete stream;
        });

    return true;
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
