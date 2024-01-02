#pragma once

#include <absl/status/status.h>
#include <httplib.h>
#include <thread>

#include "common.hh"
#include "common/config.hh"

namespace maethstro {
namespace soir {

// Handles a long-lived HTTP stream, transcoding on the fly
// raw samples into the desired format.
class HttpStream : public SampleConsumer {
 public:
  HttpStream();
  virtual ~HttpStream();

  absl::Status PushSamples(const std::string& data) override;
  absl::Status Run(httplib::Response& response);
  absl::Status Stop();

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::string stream_;
  bool running_;
};

// HTTP server that handles requests for streams. This is not meant
// to be used at scale as each connection will convert live samples
// into the desired format for now.
class HttpServer {
 public:
  HttpServer();
  ~HttpServer();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  absl::Status Run();

  int http_port_;
  std::string http_host_;

  std::unique_ptr<httplib::Server> server_;
  std::thread thread_;
};

}  // namespace soir
}  // namespace maethstro
