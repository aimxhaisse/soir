#include <absl/log/log.h>
#include <absl/strings/str_split.h>

#include "subscriber.hh"
#include "utils.hh"

namespace maethstro {
namespace matin {

Subscriber::Subscriber() {}

Subscriber::~Subscriber() {}

absl::Status Subscriber::Init(const common::Config& config) {
  user_ = config.Get<std::string>("matin.user");
  midi_grpc_host_ = config.Get<std::string>("matin.midi.grpc.host");
  midi_grpc_port_ = config.Get<int>("matin.midi.grpc.port");

  midi_stub_ = proto::Midi::NewStub(grpc::CreateChannel(
      midi_grpc_host_ + ":" + std::to_string(midi_grpc_port_),
      grpc::InsecureChannelCredentials()));
  if (!midi_stub_) {
    return absl::InternalError("Failed to create MIDI gRPC stub");
  }

  utils::InitContext(&context_, user_);

  LOG(INFO) << "Subscriber initialized with settings: " << midi_grpc_host_
            << ", " << midi_grpc_port_;

  return absl::OkStatus();
}

absl::Status Subscriber::Start() {
  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Subscriber failed: " << status.message();
    }
  });

  return absl::OkStatus();
}

absl::Status Subscriber::Stop() {
  LOG(INFO) << "Subscriber shutting down";

  context_.TryCancel();

  if (thread_.joinable()) {
    thread_.join();
  }

  LOG(INFO) << "Subscriber properly shut down";

  return absl::OkStatus();
}

absl::Status Subscriber::Run() {
  proto::MidiNotifications_Request request;

  auto stream = midi_stub_->Notifications(&context_, request);
  if (!stream) {
    return absl::InternalError("Failed to subscribe to MIDI notifications");
  }

  proto::MidiNotifications_Response response;
  while (stream->Read(&response)) {
    switch (response.notification_case()) {

      case proto::MidiNotifications_Response::kLog: {
        auto entries = absl::StrSplit(response.log().notification(), '\n');

        for (const auto& entry : entries) {
          LOG(INFO) << "\e[1;42mL I V E \e[0m\e[1;32m "
                    << response.log().source() << ">\e[0m " << entry;
        }

        break;
      }

      default:
        LOG(WARNING) << "Unknown notification type: "
                     << response.notification_case();
    }
  }

  return absl::OkStatus();
}

}  // namespace matin
}  // namespace maethstro
