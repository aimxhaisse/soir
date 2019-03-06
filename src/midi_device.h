#ifndef SOIR_MIDI_DEVICE_H
#define SOIR_MIDI_DEVICE_H

#include <RtMidi.h>

#include "status.h"

namespace soir {

using MidiMessage = std::vector<unsigned char>;
using MidiMessages = std::vector<MidiMessage>;
using MidiMnemo = std::string;

// Handles a connected MIDI device and provides facilities to poll events.
class MidiDevice {
public:
  MidiDevice(const std::string &name, int port);

  // Initializes connection to the MIDI device.
  Status Init();

  // Poll messages from the MIDI device.
  Status PollMessages(MidiMessages *messages);

private:
  // Helper to help inspecting available MIDI messages. This can be
  // used to manually build the profile for the device.
  void DumpMessage(const MidiMessage &msg) const;

  RtMidiIn midi_;
  std::string name_;
  int port_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
