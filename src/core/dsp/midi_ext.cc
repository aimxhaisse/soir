#include <absl/log/log.h>

#include "core/dsp/midi_ext.hh"

namespace {

bool chooseMidiPort(libremidi::midi_out& libremidi) {
  std::string portName;
  auto ports =
      libremidi::observer{
          {}, observer_configuration_for(libremidi.get_current_api())}
          .get_output_ports();
  unsigned int i = 0;
  std::size_t nPorts = ports.size();
  if (nPorts == 0) {
    LOG(ERROR) << "No output ports found!";
    return false;
  }

  if (nPorts == 1) {
    LOG(INFO) << "Opening port 0: " << ports[0].display_name;
  } else {
    for (i = 0; i < nPorts; i++) {
      portName = ports[i].display_name;
      LOG(INFO) << "Port " << i << ": " << portName;
    }

    do {
      std::cout << "\nChoose a port number: ";
      std::cin.ignore() >> i;
    } while (i >= nPorts);
  }

  LOG(INFO) << "Opening port " << i << ": " << ports[i].display_name;

  libremidi.open_port(ports[i]);

  return true;
}

}  // namespace

namespace neon {
namespace dsp {

MidiExt::MidiExt() {}

MidiExt::~MidiExt() {}

absl::Status MidiExt::Init() {
  if (!chooseMidiPort(midiout_)) {
    return absl::InternalError("Failed to choose MIDI port");
  }

  return absl::OkStatus();
}

void MidiExt::Render(const std::list<libremidi::message>& events,
                     AudioBuffer& buffer) {
  for (const auto& event : events) {
    midiout_.send_message(event);
  }
}

}  // namespace dsp
}  // namespace neon
