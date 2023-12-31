#pragma once

#include <absl/status/status.h>
#include <grpc++/grpc++.h>
#include <pybind11/pybind11.h>

#include "common/config.hh"
#include "engine.hh"
#include "live.grpc.pb.h"
#include "notifier.hh"

namespace maethstro {
namespace midi {

struct MidiSettings {
  std::string grpc_host;
  int grpc_port;
};

class Midi : proto::Midi::Service {
 public:
  Midi();
  ~Midi();

  absl::Status Init(const Config& config);
  absl::Status Run();
  absl::Status Wait();
  absl::Status Stop();

  grpc::Status Update(grpc::ServerContext* context,
                      const proto::MidiUpdate_Request* request,
                      proto::MidiUpdate_Response* response) override;

  grpc::Status Notifications(
      grpc::ServerContext* context,
      const proto::MidiNotifications_Request* request,
      grpc::ServerWriter<proto::MidiNotifications_Response>* writer) override;

 private:
  MidiSettings settings_;

  std::unique_ptr<Notifier> notifier_;
  std::unique_ptr<grpc::Server> grpc_;
  std::unique_ptr<Engine> engine_;
};

}  // namespace midi
}  // namespace maethstro
