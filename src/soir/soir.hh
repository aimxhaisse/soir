#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <memory>

#include "common/config.hh"
#include "engine.hh"
#include "http.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace soir {

class Soir : public proto::Soir::Service {
 public:
  Soir();
  ~Soir();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

  grpc::Status MidiEvents(
      ::grpc::ServerContext* context,
      ::grpc::ServerReader<::proto::MidiEvents_Request>* reader,
      ::proto::MidiEvents_Response* response) override;

 private:
  std::unique_ptr<HttpServer> http_server_;
  std::unique_ptr<grpc::Server> grpc_;
  std::unique_ptr<Engine> engine_;
};

}  // namespace soir
}  // namespace maethstro
