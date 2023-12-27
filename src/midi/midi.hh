#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {

struct MidiSettings {
  std::string grpc_host;
  int grpc_port;
};

class Midi : proto::Midi::Service {
 public:
  Midi();
  ~Midi();

  absl::Status Init(const Config& config);
  absl::Status Wait();
  absl::Status Stop();

  grpc::Status LiveUpdate(grpc::ServerContext* context,
                          const proto::MatinLiveUpdate_Request* request,
                          proto::MatinLiveUpdate_Response* response) override;

 private:
  MidiSettings settings_;

  std::unique_ptr<grpc::Server> grpc_;
};

}  // namespace maethstro
