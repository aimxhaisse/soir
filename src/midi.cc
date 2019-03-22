#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

#include <glog/logging.h>

#include "gfx.h"
#include "midi.h"
#include "utils.h"

namespace soir {

namespace {

constexpr const char *kMidiConfigPath = "etc/midi.yml";

constexpr const char *kMidiEventNoteOn = "note_on";

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

void MidiDevice::DumpMessage(const MidiMessage &message) const {
  uint32_t msg = 0x00000000;
  memcpy(&msg, message.data(), std::max(message.size(), sizeof(msg)));
  LOG(INFO) << "MidiDevice name=" << name_ << ", received event=0x"
            << std::setfill('0') << std::setw(8) << std::hex
            << utils::SwapEndian(msg);
}

Status MidiRule::Init(const Config &config) {
  const std::string name = config.Get<std::string>("name");
  if (name.empty()) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                 "No name found for MIDI rule (required)");
  }
  name_ = name;

  const std::string type = config.Get<std::string>("type");
  if (type == kMidiEventNoteOn) {
    type_ = NOTE_ON;
  } else {
    type_ = NONE;
  }

  const int channel = config.Get<int>("channel", -1);
  if (channel != -1) {
    channel_ = channel;
  }

  return StatusCode::OK;
}

bool MidiRule::Matches(const MidiMessage &message) const {
  uint32_t msg = 0x00000000;
  memcpy(&msg, message.data(), std::max(message.size(), sizeof(msg)));
  msg = utils::SwapEndian(msg);

  const uint8_t status = message[0];

  if (type_ && *type_ == EventType::NOTE_ON) {
    // Somehow, MIDI specs define 'note off' as a 'note on' with a
    // velocity of 0, so we need to exclude it from this.
    const bool is_note_on = (status & 0xF0) == 0x90;
    const bool has_velocity = (msg & 0x0000FF00) != 0;
    if (!(is_note_on && has_velocity)) {
      return false;
    }
  }

  const uint8_t chan = status & 0x0F;
  if (channel_ && !(*channel_ == chan)) {
    return false;
  }

  return true;
}

const std::string &MidiRule::Name() const { return name_; }

Status MidiRouter::Init() {
  MOVE_OR_RETURN(midi_config_, Config::LoadFromPath(kMidiConfigPath));

  for (const auto &device : midi_config_->GetConfigs("devices")) {
    const std::string tag = device->Get<std::string>("tag");
    if (tag.empty()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                   "Unable to find 'tag' for MIDI device (required)");
    }

    const std::string name = device->Get<std::string>("name");
    if (name.empty()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                   "Unable to find 'name' for MIDI device (required)");
    }

    const std::vector<std::unique_ptr<Config>> rules =
        device->GetConfigs("rules");
    for (const auto &rule : rules) {
      MidiRule r;
      RETURN_IF_ERROR(r.Init(*rule));
      midi_rules_[tag].push_back(r);
      LOG(INFO) << "Registered MIDI rule for device=" << tag
                << ", rule=" << r.Name();
    }

    midi_configs_[tag] = std::make_unique<Config>(*device);
    LOG(INFO) << "Registered new MIDI device tag=" << tag;
  }

  RETURN_IF_ERROR(SyncDevices());

  return StatusCode::OK;
}

Status MidiRouter::BindCallback(const std::string &mnemo, const Callback &cb) {
  midi_bindings_[mnemo].push_back(cb);

  return StatusCode::OK;
}

Status MidiRouter::ClearCallback(const Callback &callback) {
  for (auto &binding : midi_bindings_) {
    std::remove(binding.second.begin(), binding.second.end(), callback);
  }

  return StatusCode::OK;
}

Status MidiRouter::ProcessEvents() {
  MidiMessages messages;

  // If we need to optimize something, that's probably the place since
  // complexity is quite bad. For now, let's now care.
  for (auto &kv : midi_devices_) {
    RETURN_IF_ERROR(kv.second->PollMessages(&messages));
    for (const auto &message : messages) {
      for (const auto &rule : midi_rules_[kv.first]) {
        if (rule.Matches(message)) {
          LOG(INFO) << "Matching MIDI rule for device=" << kv.first
                    << ", rule=" << rule.Name();
          const std::string mnemo = kv.first + '.' + rule.Name();
          for (auto &binding : midi_bindings_[mnemo]) {
            binding.Call(message);
          }
        }
      }
    }
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
