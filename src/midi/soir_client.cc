#include <absl/log/log.h>
#include <grpc++/grpc++.h>

#include "soir_client.hh"

namespace maethstro {
namespace midi {

SoirClient::SoirClient() {}

SoirClient::~SoirClient() {}

absl::Status SoirClient::Init(const common::Config& config) {
  grpc_host_ = config.Get<std::string>("midi.soir.host");
  grpc_port_ = config.Get<int>("midi.soir.port");

  return absl::OkStatus();
}

absl::Status SoirClient::Start() {
  grpc::ChannelArguments args;

  auto channel =
      grpc::CreateCustomChannel(grpc_host_ + ":" + std::to_string(grpc_port_),
                                grpc::InsecureChannelCredentials(), args);
  soir_stub_ = proto::Soir::NewStub(channel);
  if (!soir_stub_) {
    return absl::InternalError("Failed to create Soir gRPC stub");
  }

  running_ = true;

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Soir client failed: " << status;
    }
  });

  LOG(INFO) << "Soir client running";

  return absl::OkStatus();
}

absl::Status SoirClient::Stop() {
  LOG(INFO) << "Soir client stopping";

  {
    std::unique_lock<std::mutex> lock(mutex_);
    running_ = false;
    cv_.notify_all();
  }
  thread_.join();

  LOG(INFO) << "Soir client stopped";

  return absl::OkStatus();
}

absl::Status SoirClient::SendMidiEvent(const proto::MidiEvents_Request& event) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    events_.push_back(event);
    cv_.notify_all();
  }

  return absl::OkStatus();
}

absl::Status SoirClient::Run() {
  grpc::ClientContext context;
  proto::MidiEvents_Response response;
  std::unique_ptr<grpc::ClientWriter<proto::MidiEvents_Request>> writer(
      soir_stub_->MidiEvents(&context, &response));

  while (true) {
    std::list<proto::MidiEvents_Request> events;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !events_.empty() || !running_; });

      if (!running_) {
        LOG(INFO) << "Soir client shutting down";
        break;
      }

      std::swap(events, events_);
    }

    for (const auto& event : events) {
      if (!writer->Write(event)) {
        LOG(ERROR) << "Failed to write event to Soir";
        running_ = false;
        return absl::InternalError("Failed to write event to Soir");
      }
    }
  }

  return absl::OkStatus();
}

absl::Status SoirClient::GetTracks(proto::GetTracks_Response* response) {
  grpc::ClientContext context;
  proto::GetTracks_Request request;
  auto status = soir_stub_->GetTracks(&context, request, response);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get tracks from Soir: " << status.error_message();
    return absl::InternalError("Failed to get tracks from Soir");
  }

  return absl::OkStatus();
}

}  // namespace midi
}  // namespace maethstro
