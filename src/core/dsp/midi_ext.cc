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

void MidiExt::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                     AudioBuffer& buffer) {
  // Here we need to have a dedicated thread model so that scheduled
  // events closely follow what is asked by the runtime. We can't
  // pre-render those in advance.
  midi_stack_.AddEvents(events);
  std::list<MidiEventAt> events_at;
  midi_stack_.EventsAtTick(tick, events_at);

  for (const auto& event_at : events_at) {
    midiout_.send_message(event_at.Msg());
  }
}

}  // namespace dsp
}  // namespace neon
