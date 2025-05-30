#pragma once

#include <SDL2/SDL.h>
#include <absl/status/status.h>
#include <condition_variable>
#include <libremidi/libremidi.hpp>
#include <list>
#include <mutex>
#include <thread>

#include "core/audio_buffer.hh"
#include "core/inst/instrument.hh"
#include "core/midi_stack.hh"

namespace soir {
namespace inst {

class MidiExt : public Instrument {
 public:
  MidiExt();
  ~MidiExt();

  absl::Status Init(const std::string& settings, SampleManager* sample_manager,
                    Controls* controls);

  Type GetType() const { return Type::MIDI_EXT; }
  std::string GetName() const { return "MidiExt"; }

  absl::Status Start();
  absl::Status Run();
  absl::Status Stop();

  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer);

  // Get the list of available audio input devices (device number and associated name).
  static absl::Status GetAudioDevices(
      std::vector<std::pair<int, std::string>>* out);

  // Get the list of available MIDI output devices (device number and associated name).
  static absl::Status GetMidiDevices(
      std::vector<std::pair<int, std::string>>* out);

 private:
  void ScheduleMidiEvents(const absl::Time& next_block_at);
  void FillAudioBuffer(Uint8* stream, int len);
  void WaitForInitialTick();

  // Current configuration as set from live coding. This is a cache
  // used to know upon update if we need to re-initialize the
  // device/chans, etc.
  std::string settings_ = "";
  std::string settings_midi_out_ = "";
  std::string settings_audio_in_ = "";
  std::vector<int> settings_chans_ = {0, 1};

  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread thread_;
  bool stop_ = false;
  std::vector<Uint8> consumed_;
  std::list<AudioBuffer> buffers_;
  SampleTick current_tick_ = 0;

  // Underlying devices to which we are connected to.
  libremidi::midi_out midi_out_;
  SDL_AudioDeviceID audio_out_ = -1;
  int audio_out_chans_ = -1;

  MidiStack midi_stack_;
};

}  // namespace inst
}  // namespace soir
