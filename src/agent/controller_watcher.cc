#include <absl/log/log.h>

#include "agent/controller_watcher.hh"

namespace soir {
namespace agent {

ControllerWatcher::ControllerWatcher() {}

ControllerWatcher::~ControllerWatcher() {}

absl::Status ControllerWatcher::Init(const utils::Config& config,
                                     proto::Soir::Stub* stub) {
  soir_stub_ = stub;

  LOG(INFO) << "Controller watcher initialized";

  return absl::OkStatus();
}

absl::Status ControllerWatcher::Start() {
  midi_obs_ = std::make_unique<libremidi::observer>();

  midi_in_ =
      std::make_unique<libremidi::midi_in>(libremidi::input_configuration{
          .on_message =
              [this](const libremidi::message& msg) {
                proto::PushMidiEventsRequest update;
                proto::PushMidiEventsResponse response;

                update.set_midi_payload(msg.bytes.data(), msg.bytes.size());

                grpc::ClientContext context;
                grpc::Status status =
                    soir_stub_->PushMidiEvents(&context, update, &response);
                if (!status.ok()) {
                  LOG(WARNING) << "Failed to send MIDI controller message: "
                               << status.error_message();
                }
              },
          .ignore_sensing = false,
      });

  auto inputs = midi_obs_->get_input_ports();
  for (const auto& input : inputs) {
    LOG(INFO) << "Opening port: " << input.port_name;
    midi_in_->open_port(input);
  }

  return absl::OkStatus();
}

absl::Status ControllerWatcher::Stop() {
  midi_in_.reset();
  midi_obs_.reset();

  return absl::OkStatus();
}

}  // namespace agent
}  // namespace soir
