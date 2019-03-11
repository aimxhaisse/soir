#include <iostream>

#include <glog/logging.h>

#include "midi.h"

namespace soir {

MidiDevice::MidiDevice(std::unique_ptr<RtMidiIn> rtmidi,
                       const std::string &name, int port)
    : rtmidi_(std::move(rtmidi)), name_(name), port_(port) {}

Status MidiDevice::Init() {
  try {
    rtmidi_->openPort(port_);
  } catch (RtMidiError &error) {
    RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                 "Unable to open MIDI port, port="
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
  bool continue_registering_devices = false;
  do {
    try {
      std::unique_ptr<RtMidiIn> rtmidi = std::make_unique<RtMidiIn>();
      for (int i = 0; i < rtmidi->getPortCount(); ++i) {
        const std::string name = rtmidi->getPortName(i);
        if (midi_devices_.find(name) != midi_devices_.end()) {
          continue;
        }
        continue_registering_devices = true;
        auto device = std::make_unique<MidiDevice>(std::move(rtmidi), name, i);
        Status status = device->Init();
        if (status == StatusCode::OK) {
          midi_devices_[name] = std::move(device);
          LOG(INFO) << "New MIDI device connected, name=" << name;
        } else {
          LOG(WARNING) << "Failed to connect MIDI device, name=" << name
                       << ", status=" << status;
        }
      }
    } catch (RtMidiError &error) {
      RETURN_ERROR(StatusCode::INTERNAL_MIDI_ERROR,
                   "Unable to initialize MIDI, error=" << error.what());
    }
  } while (continue_registering_devices);
  return StatusCode::OK;
}

} // namespace soir
