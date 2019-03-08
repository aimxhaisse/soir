#ifndef SOIR_MIDI_H
#define SOIR_MIDI_H

#include <map>

#include <RtMidi.h>

#include "status.h"

namespace soir {

using MidiMessage = std::vector<unsigned char>;
using MidiMessages = std::vector<MidiMessage>;
using MidiMnemo = std::string;

// Abstract class that represents a socket to a MIDI device.  The
// goal of this class is to wrap RtMidi's library to only expose what
// we use, in a safe way (i.e: preventing all exceptions from
// propagating) as well as to create mock devices in tests.
class MidiSocket {
public:
  virtual ~MidiSocket() {}

  virtual Status OpenPort(int port) { return StatusCode::OK; }
  virtual Status GetMessage(MidiMessage *message) { return StatusCode::OK; }
};

// Implementation of a MidiSocket using RtMidi's library.
class RtMidiSocket : public MidiSocket {
public:
  Status OpenPort(int port);
  Status GetMessage(MidiMessage *message);

private:
  RtMidiIn midi_;
};

// Handles a connected MIDI device and provides facilities to poll events.
class MidiDevice {
public:
  // Creates a new MIDI device from a socket.
  explicit MidiDevice(const std::string &name,
                      std::unique_ptr<MidiSocket> socket);

  // Whether or not to enable verbose debugging of incoming events. By
  // default this is false.
  void SetDebugging(bool debugging);

  // Poll messages from the MIDI device.
  Status PollMessages(MidiMessages *messages);

private:
  // Helper to help inspecting available MIDI messages. This can be
  // used to manually build the profile for the device.
  void DumpMessage(const MidiMessage &msg) const;

  const std::string name_;
  std::unique_ptr<MidiSocket> socket_;
  bool debugging_;
};

// MidiRouter scans available MIDI devices, polls events from those,
// convert them to a MidiEvent, and route those to interested mods.
class MidiRouter {
public:
  // Check if a device is already known or not.
  bool HasDevice(const std::string &name) const;

  // Removes a devices from the router.
  bool RemoveDevice(const std::string &name);

  // Registers a new device in the map, returns INTERNAL_MIDI_ERROR if
  // a MIDI device with this name already exists.
  Status RegisterDevice(const std::string &name,
                        std::unique_ptr<MidiDevice> device);

  // Poll MIDI messages from all known MIDI devices, if a MIDI device
  // returns an error (disconnected), it is removed from the list of map
  // of devices.
  Status ProcessEvents();

private:
  // Map of MIDI device names to corresponding attached MIDI devices.
  std::map<std::string, std::unique_ptr<MidiDevice>> midi_devices_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
