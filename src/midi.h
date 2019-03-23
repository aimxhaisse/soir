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

class MidiMessage {
public:
  explicit MidiMessage(const std::vector<unsigned char> data);

  static constexpr const uint32_t STATUS_MASK = 0xF0000000;
  static constexpr const uint32_t CHAN_MASK = 0x0F000000;
  static constexpr const uint32_t VEL_MASK = 0x0000FF00;

  static constexpr const uint8_t NOTE_ON = 0x90;
  static constexpr const uint8_t NOTE_OFF = 0x80;

  inline uint32_t Raw() const { return msg_; }

  inline uint8_t Status() const { return (msg_ & STATUS_MASK) >> 24; }
  inline uint8_t Channel() const { return (msg_ & CHAN_MASK) >> 28; }
  inline uint8_t Vel() const { return (msg_ & VEL_MASK) >> 8; }

  inline bool NoteOn() const { return Status() == NOTE_ON && Vel() != 0; }
  inline bool NoteOff() const {
    return Status() == NOTE_OFF || (Status() == NOTE_ON && Vel() == 0);
  }

private:
  uint32_t msg_;
};

class MidiDevice {
public:
  MidiDevice(const std::string &name, int port);

  Status Init();
  void SetDebugging(bool debugging);
  Status PollMessages(MidiMessages *messages);
  const std::string &Name() const;

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
    NOTE_OFF,
  };

  EventType type_ = EventType::NONE;
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
  Status InitRules();
  Status SyncDevices();

  std::unique_ptr<Config> midi_config_;
  std::unordered_map<std::string, std::unique_ptr<Config>> midi_configs_;
  std::unordered_map<std::string, std::unique_ptr<MidiDevice>> midi_devices_;
  std::unordered_map<std::string, std::vector<MidiRule>> midi_rules_;
  std::unordered_map<MidiMnemo, std::vector<Callback>> midi_bindings_;
};

} // namespace soir

#endif // SOIR_MIDI_DEVICE_H
