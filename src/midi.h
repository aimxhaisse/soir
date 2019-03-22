#ifndef SOIR_MIDI_H
#define SOIR_MIDI_H

#include <cstdint>
#include <experimental/optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <RtMidi.h>

#include "common.h"
#include "config.h"
#include "status.h"

namespace soir {

class Callback;

class MidiDevice {
public:
  MidiDevice(const std::string &name, int port);

  Status Init();
  void SetDebugging(bool debugging);
  Status PollMessages(MidiMessages *messages);

private:
  void DumpMessage(const MidiMessage &msg) const;

  std::unique_ptr<RtMidiIn> rtmidi_;
  const std::string name_;
  const int port_;
  bool debugging_;
};

class MidiRule {
public:
  Status Init(const Config &config);
  bool Matches(const MidiMessage &message) const;
  const std::string &Name() const;

private:
  enum EventType {
    NONE,
    NOTE_ON,
  };

  std::experimental::optional<EventType> type_;
  std::experimental::optional<uint8_t> channel_;
  std::string name_;
};

class MidiRouter {
public:
  Status Init();
  Status BindCallback(const MidiMnemo &mnemo, const Callback &cb);
  Status ClearCallback(const Callback &callback);
  Status ProcessEvents();

private:
  Status SyncDevices();

  std::unique_ptr<Config> midi_config_;
  std::unordered_map<std::string, std::unique_ptr<Config>> midi_configs_;
  std::unordered_map<std::string, std::unique_ptr<MidiDevice>> midi_devices_;
  std::unordered_map<std::string, std::vector<MidiRule>> midi_rules_;
  std::unordered_map<MidiMnemo, std::vector<Callback>> midi_bindings_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
