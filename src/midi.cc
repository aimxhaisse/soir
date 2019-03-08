#include <iostream>

#include <glog/logging.h>

#include "midi.h"

namespace soir {

Status RtMidiSocket::OpenPort(int port) {
  try {
    midi_.openPort(port);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to open MIDI port '" << port << "': " << error.what());
  }
  return StatusCode::OK;
}

Status RtMidiSocket::GetMessage(MidiMessage *message) {
  try {
    midi_.getMessage(message);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to get MIDI message: " << error.what());
  }
  return StatusCode::OK;
}

MidiDevice::MidiDevice(const std::string &name,
                       std::unique_ptr<MidiSocket> socket)
    : name_(name), socket_(std::move(socket)), debugging_(false) {}

void MidiDevice::SetDebugging(bool debugging) { debugging_ = debugging; }

Status MidiDevice::PollMessages(MidiMessages *messages) {
  while (true) {
    MidiMessage message;
    RETURN_IF_ERROR(socket_->GetMessage(&message));
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
              << part;
  }
}

bool MidiRouter::HasDevice(const std::string &name) const {
  return midi_devices_.find(name) != midi_devices_.end();
}

bool MidiRouter::RemoveDevice(const std::string &name) {
  return midi_devices_.erase(name);
}

Status MidiRouter::RegisterDevice(const std::string &name,
                                  std::unique_ptr<MidiDevice> device) {
  if (HasDevice(name)) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Can't register MIDI device, '" << name
                                                 << "' is already registered");
  }

  midi_devices_[name] = std::move(device);
  LOG(INFO) << "MIDI device '" << name << "' has been registered";

  return StatusCode::OK;
}

Status MidiRouter::ProcessEvents() {
  MidiMessages messages;
  for (auto &kv : midi_devices_) {
    RETURN_IF_ERROR(kv.second->PollMessages(&messages));
    messages.clear();
  }
  return StatusCode::OK;
}

} // namespace soir
