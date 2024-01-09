#include <absl/log/log.h>

#include "controller_watcher.hh"
#include "utils.hh"

namespace maethstro {
namespace matin {

ControllerWatcher::ControllerWatcher() {}

ControllerWatcher::~ControllerWatcher() {}

absl::Status ControllerWatcher::Init(const common::Config& config,
                                     proto::Midi::Stub* stub) {
  midi_stub_ = stub;
  user_ = config.Get<std::string>("matin.user");

  LOG(INFO) << "Controller watcher initialized";

  return absl::OkStatus();
}

absl::Status ControllerWatcher::Start() {
  midi_obs_ = std::make_unique<libremidi::observer>();

  midi_in_ =
      std::make_unique<libremidi::midi_in>(libremidi::input_configuration{
          .on_message =
              [this](const libremidi::message& msg) {
                proto::MidiUpdate_Request update;
                proto::MidiUpdate_Response response;
                proto::MidiUpdate_Midi* midi_update = update.mutable_midi();

                midi_update->set_payload(msg.bytes.data(), msg.bytes.size());

                grpc::ClientContext context;
                utils::InitContext(&context, user_);

                grpc::Status status =
                    midi_stub_->Update(&context, update, &response);
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

}  // namespace matin
}  // namespace maethstro
