#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp/midi_ext.hh"

namespace {

bool UseMidiPort(int port, libremidi::midi_out& libremidi) {
  auto ports =
      libremidi::observer{
          {}, observer_configuration_for(libremidi.get_current_api())}
          .get_output_ports();

  if (port >= ports.size()) {
    LOG(ERROR) << "MIDI port number out of range!";
    return false;
  }

  LOG(INFO) << "Opening port " << port << ": " << ports[port].display_name;

  libremidi.open_port(ports[port]);

  if (!libremidi.is_port_open()) {
    LOG(ERROR) << "Failed to open MIDI port " << port;
    return false;
  }

  return true;
}

}  // namespace

namespace neon {
namespace dsp {

MidiExt::MidiExt() {}

MidiExt::~MidiExt() {}

absl::Status MidiExt::Init(const std::string& settings) {
  if (settings == last_settings_) {
    return absl::OkStatus();
  }

  rapidjson::Document params;
  params.Parse(settings.c_str());
  auto midi_port = params["midi_device"].GetInt();

  LOG(INFO) << "Trying to open MIDI port " << midi_port << "...";

  if (!UseMidiPort(midi_port, midiout_)) {
    return absl::InternalError("Failed to use MIDI port");
  }

  last_settings_ = settings;

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
