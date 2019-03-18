#ifndef SOIR_MIDI_H
#define SOIR_MIDI_H

#include <cstdint>
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

// Represents a MIDI rule. This is a simplification (at least for
// now), we only support MIDI messages up to 4 bytes (this allows
// simple matches). 8 bytes would be a simple move but it makes
// writing rules more verbose.
//
// This representation is similar to networking. For instance, to
// match MIDI events "note on channel 9", you want to select the
// first byte so the mask is 0xFF000000, you want it on channel 0x08
// for event type 0x90, so the rule is 0xFF000000/0x98000000.
class MidiRule {
public:
  // Initializes the rule, returns an error if the filter is not valid.
  Status Init(const std::string &name, const std::string &filter);

  // Whether or not this rule matches the incoming MIDI message.
  bool Matches(const MidiMessage &message) const;

  // Name of the rule.
  const std::string &Name() const;

private:
  // Mask of the rule.
  uint32_t mask_;

  // Value to match.
  uint32_t value_;

  // Name of the rule.
  std::string name_;
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
