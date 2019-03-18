#ifndef SOIR_MIDI_H
#define SOIR_MIDI_H

#include <unordered_map>
#include <utility>
#include <vector>

#include <RtMidi.h>

#include "config.h"
#include "status.h"

namespace soir {

using MidiMessage = std::vector<unsigned char>;
using MidiMessages = std::vector<MidiMessage>;
using MidiMnemo = std::string;

// Handles a connected MIDI device and provides facilities to poll events.
class MidiDevice {
public:
  // Creates a MIDI device from a port.
  MidiDevice(const std::string &name, int port);

  // Initializes connection to the MIDI device.
  Status Init();

  // Whether or not to enable verbose debugging of incoming events. By
  // default this is false.
  void SetDebugging(bool debugging);

  // Poll messages from the MIDI device.
  Status PollMessages(MidiMessages *messages);

private:
  // Helper to help inspecting available MIDI messages. This can be
  // used to manually build the profile for the device.
  void DumpMessage(const MidiMessage &msg) const;

  std::unique_ptr<RtMidiIn> rtmidi_;
  const std::string name_;
  const int port_;
  bool debugging_;
};

// MidiRouter scans available MIDI devices, polls events from those,
// convert them to a MidiEvent, and route those to interested mods.
class MidiRouter {
public:
  // Initializes the MIDI router.
  Status Init();

  // Poll MIDI messages from all known MIDI devices, if a MIDI device
  // returns an error (disconnected), it is removed from the list of map
  // of devices.
  Status ProcessEvents();

private:
  // Synchronizes connected MIDI devices.
  Status SyncDevices();

  // Main MIDI config file, contains specific device profiles.
  std::unique_ptr<Config> midi_config_;

  // Map of configurations of MIDI devices, this is static and
  // initialized at startup.
  std::unordered_map<std::string, std::unique_ptr<Config>> midi_configs_;

  // Map of MIDI device names to corresponding attached MIDI devices.
  std::unordered_map<std::string, std::unique_ptr<MidiDevice>> midi_devices_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
