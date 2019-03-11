#include <iostream>
#include <map>

#include <glog/logging.h>

#include "midi.h"

namespace soir {

MidiDevice::MidiDevice(const std::string &name, int port)
    : name_(name), port_(port) {}

Status MidiDevice::Init() {
  try {
    rtmidi_ = std::make_unique<RtMidiIn>();
    rtmidi_->openPort(port_);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to connect to MIDI device port="
                     << port_ << ", name=" << name_
                     << ", error=" << error.what());
  }
  return StatusCode::OK;
}

void MidiDevice::SetDebugging(bool debugging) { debugging_ = debugging; }

Status MidiDevice::PollMessages(MidiMessages *messages) {
  while (true) {
    MidiMessage message;
    try {
      rtmidi_->getMessage(&message);
    } catch (RtMidiError &error) {
      RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                   "Unable to get MIDI messages, port="
                       << port_ << ", name=" << name_
                       << ", error=" << error.what());
    }
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
    LOG(INFO) << "MidiDevice name=" << name_ << ", received event=" << std::hex
              << part;
  }
}

Status MidiRouter::ProcessEvents() {
  MidiMessages messages;
  for (auto &kv : midi_devices_) {
    RETURN_IF_ERROR(kv.second->PollMessages(&messages));
    messages.clear();
  }
  return StatusCode::OK;
}

Status MidiRouter::SyncDevices() {
  // Get connected devices.
  std::unique_ptr<RtMidiIn> rtmidi = std::make_unique<RtMidiIn>();
  std::map<std::string, int> devices;
  for (int i = 0; i < rtmidi->getPortCount(); ++i) {
    devices[rtmidi->getPortName(i)] = i;
  }

  // First pass, register new devices.
  for (const auto &it : devices) {
    const std::string &name = it.first;
    const int port = it.second;
    if (midi_devices_.find(name) != midi_devices_.end()) {
      continue;
    }
    auto device = std::make_unique<MidiDevice>(name, port);
    Status status = device->Init();
    if (status == StatusCode::OK) {
      LOG(INFO) << "New MIDI device connected, name=" << name;
      midi_devices_[name] = std::move(device);
    }
  }

  // Second pass, delete disconnected devices.
  auto it = midi_devices_.begin();
  while (it != midi_devices_.end()) {
    if (devices.find(it->first) == devices.end()) {
      LOG(INFO) << "MIDI device disconnected, name=" << it->first;
      it = midi_devices_.erase(it);
    } else {
      ++it;
    }
  }

  return StatusCode::OK;
}

} // namespace soir
