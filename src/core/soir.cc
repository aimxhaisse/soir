#include <absl/log/log.h>

#include "soir.hh"

namespace soir {

Soir::Soir()
    : dsp_(std::make_unique<engine::Engine>()),
      rt_(std::make_unique<rt::Engine>()),
      notifier_(std::make_unique<rt::Notifier>()) {}

Soir::~Soir() {}

absl::Status Soir::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing Soir";

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

  auto grpc_host = config.Get<std::string>("soir.grpc.host");
  auto grpc_port = config.Get<int>("soir.grpc.port");

  grpc::ServerBuilder builder;

  builder.AddListeningPort(grpc_host + ":" + std::to_string(grpc_port),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(this);

  grpc_ = builder.BuildAndStart();
  if (!grpc_) {
    return absl::InternalError("Unable to start grpc server");
  }

  LOG(INFO) << "Soir listening on " << grpc_host << ":" << grpc_port;

  return absl::OkStatus();
}

absl::Status Soir::Start() {
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

absl::Status Soir::Stop() {
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

grpc::Status Soir::PushMidiEvents(grpc::ServerContext* context,
                                  const proto::PushMidiEventsRequest* request,
                                  proto::PushMidiEventsResponse* response) {
  libremidi::message msg;

  msg.bytes = libremidi::midi_bytes(request->midi_payload().begin(),
                                    request->midi_payload().end());

  // Here we add a block size delay to be sure we don't pad events to
  // the beginning of block processing: we want this to be scheduled
  // in the nearest possible future but not in the past, otherwise
  // those events will be considered late and immediately scheduled on
  // tick 0 of the block.
  auto delay =
      absl::Microseconds((soir::engine::kBlockSize * 1e6) / engine::kSampleRate);
  dsp_->PushMidiEvent(MidiEventAt("*", msg, absl::Now()));

  return grpc::Status::OK;
}

grpc::Status Soir::PushCodeUpdate(grpc::ServerContext* context,
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

grpc::Status Soir::GetLogs(grpc::ServerContext* context,
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

}  // namespace soir
