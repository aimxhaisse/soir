#pragma once

#include <grpc++/grpc++.h>

#include "core/engine/engine.hh"
#include "core/rt/runtime.hh"
#include "core/rt/notifier.hh"
#include "soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {

class Soir : proto::Soir::Service {
 public:
  Soir();
  ~Soir();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  grpc::Status PushMidiEvents(grpc::ServerContext* context,
                              const proto::PushMidiEventsRequest* request,
                              proto::PushMidiEventsResponse* response) override;

  grpc::Status PushCodeUpdate(grpc::ServerContext* context,
                              const proto::PushCodeUpdateRequest* request,
                              proto::PushCodeUpdateResponse* response) override;

  grpc::Status GetLogs(
      grpc::ServerContext* context, const proto::GetLogsRequest* request,
      grpc::ServerWriter<proto::GetLogsResponse>* writer) override;

 private:
  std::unique_ptr<engine::Engine> dsp_;
  std::unique_ptr<rt::Runtime> rt_;
  std::unique_ptr<rt::Notifier> notifier_;
  std::unique_ptr<grpc::Server> grpc_;
};

}  // namespace soir
