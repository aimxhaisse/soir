#include "audio/audio_http_server.hh"

#include "absl/log/log.h"
#include "audio/audio_stream.hh"
#include "httplib.h"

namespace soir {
namespace audio {

AudioHttpServer::AudioHttpServer() = default;

AudioHttpServer::~AudioHttpServer() { Stop().IgnoreError(); }

absl::Status AudioHttpServer::Start(AudioStream* stream, const std::string& host,
                                    int port) {
  if (running_) {
    return absl::OkStatus();
  }

  server_ = std::make_unique<httplib::Server>();

  server_->Get("/stream.opus",
               [stream](const httplib::Request&, httplib::Response& res) {
                 auto header_pages = stream->GetHeaderPages();
                 size_t client_offset = 0;
                 bool headers_sent = false;

                 res.set_chunked_content_provider(
                     "audio/ogg",
                     [stream, header_pages, client_offset, headers_sent](
                         size_t, httplib::DataSink& sink) mutable -> bool {
                       if (!sink.is_writable()) {
                         return false;
                       }

                       if (!headers_sent) {
                         if (!header_pages.empty()) {
                           sink.write(
                               reinterpret_cast<const char*>(
                                   header_pages.data()),
                               header_pages.size());
                         }
                         headers_sent = true;
                       }

                       auto result = stream->Read(client_offset, 500);
                       if (!result.data.empty()) {
                         client_offset = result.new_offset;
                         return sink.write(
                             reinterpret_cast<const char*>(result.data.data()),
                             result.data.size());
                       }

                       return true;
                     });
               });

  running_ = true;

  server_thread_ = std::thread([this, host, port]() {
    LOG(INFO) << "Audio HTTP server listening on " << host << ":" << port;
    if (!server_->listen(host, port)) {
      LOG(ERROR) << "Audio HTTP server failed to listen on " << host << ":"
                 << port;
    }
  });

  return absl::OkStatus();
}

absl::Status AudioHttpServer::Stop() {
  if (!running_) {
    return absl::OkStatus();
  }

  server_->stop();

  if (server_thread_.joinable()) {
    server_thread_.join();
  }

  running_ = false;

  LOG(INFO) << "Audio HTTP server stopped";
  return absl::OkStatus();
}

}  // namespace audio
}  // namespace soir
