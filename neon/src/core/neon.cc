#include <absl/log/log.h>

#include "neon.hh"

namespace neon {

Neon::Neon()
    : dsp_(std::make_unique<dsp::Engine>()),
      rt_(std::make_unique<rt::Engine>()),
      notifier_(std::make_unique<rt::Notifier>()) {}

Neon::~Neon() {}

absl::Status Neon::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing Neon";

  auto status = notifier_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize notifier: " << status;
    return status;
  }

  status = dsp_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize DSP engine: " << status;
    return status;
  }

  status = rt_->Init(config, dsp_.get(), notifier_.get());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize RT engine: " << status;
    return status;
  }

  auto grpc_host = config.Get<std::string>("grpc.host");
  auto grpc_port = config.Get<int>("grpc.port");

  grpc::ServerBuilder builder;

  builder.AddListeningPort(grpc_host + ":" + std::to_string(grpc_port),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(this);

  grpc_ = builder.BuildAndStart();
  if (!grpc_) {
    return absl::InternalError("Unable to start grpc server");
  }

  LOG(INFO) << "Neon listening on " << grpc_host << ":" << grpc_port;

  return absl::OkStatus();
}

absl::Status Neon::Start() {
  auto status = notifier_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start notifier: " << status;
    return status;
  }

  status = dsp_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start DSP engine: " << status;
    return status;
  }

  status = rt_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start RT engine: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Neon::Stop() {
  auto status = rt_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop RT engine: " << status;
    return status;
  }

  status = dsp_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop DSP engine: " << status;
    return status;
  }

  status = notifier_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop notifier: " << status;
    return status;
  }

  grpc_->Shutdown();
  grpc_->Wait();

  return absl::OkStatus();
}

grpc::Status Neon::PushMidiEvents(grpc::ServerContext* context,
                                  const proto::PushMidiEventsRequest* request,
                                  proto::PushMidiEventsResponse* response) {
  libremidi::message msg;

  msg.bytes = libremidi::midi_bytes(request->midi_payload().begin(),
                                    request->midi_payload().end());

  dsp_->PushMidiEvent(msg);

  return grpc::Status::OK;
}

grpc::Status Neon::PushCodeUpdate(grpc::ServerContext* context,
                                  const proto::PushCodeUpdateRequest* request,
                                  proto::PushCodeUpdateResponse* response) {
  auto status = rt_->PushCodeUpdate(request->code());
  if (!status.ok()) {
    LOG(ERROR) << "Failed to push code update: " << status;
    return grpc::Status(grpc::StatusCode::INTERNAL,
                        "Unable to push code update");
  }

  return grpc::Status::OK;
}

grpc::Status Neon::GetLogs(grpc::ServerContext* context,
                           const proto::GetLogsRequest* request,
                           grpc::ServerWriter<proto::GetLogsResponse>* writer) {
  LOG(INFO) << "New log subscription from " << context->peer();

  rt::Subscriber sub(*writer);

  auto status = notifier_->Register(&sub);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to register subscriber: " << status;
    return grpc::Status(grpc::StatusCode::INTERNAL,
                        "Unable to register subscriber");
  }

  status = sub.Wait();
  if (!status.ok()) {
    LOG(WARNING) << "Subscriber disconnected: " << status;
  } else {
    LOG(INFO) << "Subscriber disconnected";
  }

  status = notifier_->Unregister(&sub);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to unregister subscriber: " << status;
  }

  return grpc::Status::OK;
}

}  // namespace neon
