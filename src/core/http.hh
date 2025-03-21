#pragma once

#include <absl/status/status.h>
#include <httplib.h>
#include <thread>

#include "core/dsp.hh"
#include "core/engine.hh"
#include "core/http_stream.hh"
#include "utils/config.hh"
#include "utils/misc.hh"

namespace soir {


class Engine;

// HTTP server that handles requests for streams. This is not meant
// to be used at scale as each connection will convert live samples
// into the desired format for now.
class HttpServer {
 public:
  HttpServer();
  ~HttpServer();

  absl::Status Init(const utils::Config& config, Engine* engine);
  absl::Status Start();
  absl::Status Stop();

 private:
  absl::Status Run();

  Engine* engine_;
  int http_port_;
  std::string http_host_;

  std::unique_ptr<httplib::Server> server_;
  std::thread thread_;
};


}  // namespace soir
