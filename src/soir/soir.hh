#pragma once

#include <absl/status/status.h>
#include <memory>

#include "common/config.hh"
#include "engine.hh"
#include "http.hh"

namespace maethstro {
namespace soir {

class Soir {
 public:
  Soir();
  ~Soir();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  std::unique_ptr<HttpServer> http_server_;
  std::unique_ptr<Engine> engine_;
};

}  // namespace soir
}  // namespace maethstro
