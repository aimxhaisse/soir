#include <iostream>
#include <map>

#include <glog/logging.h>

#include "midi.h"
#include "utils.h"

namespace soir {

namespace {

// Relative path to the midi profiles configuration file.
constexpr const char *kMidiConfigPath = "etc/midi.yml";

// Default value for device debugging -- devices.<x>.debug
constexpr const bool kDefaultDebugDevice = false;

} // namespace

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

Status MidiRule::Init(const std::string &name, const std::string &filter) {
  const std::vector<std::string> parts = utils::StringSplit(filter, '/');
  if (parts.size() != 2) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                 "Invalid MIDI rule: filter does not follow X/Y format, filter="
                     << filter);
  }

  mask_ = utils::SwapEndian(std::strtoul(parts[0].c_str(), 0, 16));
  value_ = utils::SwapEndian(std::strtoul(parts[1].c_str(), 0, 16));

  name_ = name;

  return StatusCode::OK;
}

bool MidiRule::Matches(const MidiMessage &message) const {
  // Kind-of hackish but efficient way to match a midi message against
  // a midi event.
  uint32_t msg = 0x00000000;
  memcpy(&msg, message.data(), std::max(message.size(), sizeof(msg)));
  return ((msg & mask_) == value_);
}

const std::string &MidiRule::Name() const { return name_; }

Status MidiRouter::Init() {
  MOVE_OR_RETURN(midi_config_, Config::LoadFromPath(kMidiConfigPath));

  // Register MIDI devices from config file.
  for (const auto &device : midi_config_->GetConfigs("devices")) {
    const std::string tag = device->Get<std::string>("tag");
    if (tag.empty()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                   "Unable to find 'tag' for MIDI device (required)");
    }
    midi_configs_[tag] = std::make_unique<Config>(*device);
    LOG(INFO) << "Registered new MIDI device tag=" << tag;
  }

  RETURN_IF_ERROR(SyncDevices());

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

    auto config_it = midi_configs_.find(name);
    if (config_it == midi_configs_.end()) {
      LOG(INFO) << "Ignored connected MIDI device name=" << name
                << " (unknown device)";
      continue;
    }

    auto device = std::make_unique<MidiDevice>(name, port);
    Status status = device->Init();

    if (status != StatusCode::OK) {
      LOG(WARNING) << "Failed to connect MIDI device, name=" << name
                   << ", status=" << status;
      continue;
    }

    const Config &config = *config_it->second;
    device->SetDebugging(config.Get<bool>("debug", kDefaultDebugDevice));

    LOG(INFO) << "New MIDI device connected, name=" << name;
    midi_devices_[name] = std::move(device);
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
