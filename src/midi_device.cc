#include <iostream>

#include <glog/logging.h>

#include "midi_device.h"

namespace soir {

MidiDevice::MidiDevice(const std::string &name, int port)
    : name_(name), port_(port) {}

Status MidiDevice::Init() {
  midi_.openPort(port_);
  return StatusCode::OK;
}

Status MidiDevice::PollMessages(MidiMessages *messages) {
  MidiMessage message;

  while (true) {
    message.clear();
    midi_.getMessage(&message);
    if (message.empty()) {
      break;
    }
    DumpMessage(message);
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
