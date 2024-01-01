#pragma once

#include <absl/status/status.h>
#include <httplib.h>
#include <thread>

#include "common/config.hh"

namespace maethstro {
namespace soir {

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
