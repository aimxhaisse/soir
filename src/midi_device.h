#ifndef SOIR_MIDI_DEVICE_H
#define SOIR_MIDI_DEVICE_H

#include <RtMidi.h>

#include "status.h"

namespace soir {

using MidiMessage = std::vector<unsigned char>;
using MidiMessages = std::vector<MidiMessage>;
using MidiMnemo = std::string;

// Abstract class that represents a connection to a MIDI device.  The
// goal of this class is to wrap RtMidi's library to only expose what
// we use, in a safe way (i.e: preventing all exceptions from
// propagating) as well as to create mock devices in tests.
class MidiConnection {
public:
  virtual ~MidiConnection() {}

  virtual Status OpenPort(int port) { return StatusCode::OK; }
  virtual Status GetMessage(MidiMessage *message) { return StatusCode::OK; }
};

// Implementation of a MidiConnection using RtMidi's library.
class RtMidiConnection : public MidiConnection {
public:
  Status OpenPort(int port);
  Status GetMessage(MidiMessage *message);

private:
  RtMidiIn midi_;
};

// Handles a connected MIDI device and provides facilities to poll events.
class MidiDevice {
public:
  MidiDevice(const std::string &name, int port);

  // Whether or not to enable verbose debugging of incoming events. By
  // default this is false.
  void SetDebugging(bool debugging);

  // Initializes connection to the MIDI device.
  Status Init();

  // Poll messages from the MIDI device.
  Status PollMessages(MidiMessages *messages);

private:
  // Helper to help inspecting available MIDI messages. This can be
  // used to manually build the profile for the device.
  void DumpMessage(const MidiMessage &msg) const;

  MidiConnection midi_;
  std::string name_;
  int port_;
  bool debugging_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
