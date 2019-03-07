#include <iostream>

#include <glog/logging.h>

#include "midi_device.h"

namespace soir {

Status RtMidiConnection::OpenPort(int port) {
  try {
    midi_.openPort(port);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to open MIDI port '" << port << "': " << error.what());
  }
  return StatusCode::OK;
}

Status RtMidiConnection::GetMessage(MidiMessage *message) {
  try {
    midi_.getMessage(message);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to get MIDI message: " << error.what());
  }
  return StatusCode::OK;
}

MidiDevice::MidiDevice(const std::string &name, int port)
    : name_(name), port_(port), debugging_(false) {}

void MidiDevice::SetDebugging(bool debugging) { debugging_ = debugging; }

Status MidiDevice::Init() { return midi_.OpenPort(port_); }

Status MidiDevice::PollMessages(MidiMessages *messages) {
  while (true) {
    MidiMessage message;
    RETURN_IF_ERROR(midi_.GetMessage(&message));
    if (message.empty()) {
      break;
    }
    if (debugging_) {
      DumpMessage(message);
    }
    messages->push_back(message);
  }

  return StatusCode::OK;
}

void MidiDevice::DumpMessage(const MidiMessage &msg) const {
  for (const auto &part : msg) {
    LOG(INFO) << "MidiDevice name=" << name_ << " received event=" << std::hex
              << part << " at midi port=" << port_;
  }
}

} // namespace soir
