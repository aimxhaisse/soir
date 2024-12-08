#pragma once

#include <SDL2/SDL.h>
#include <absl/status/status.h>
#include <condition_variable>
#include <libremidi/libremidi.hpp>
#include <list>
#include <mutex>
#include <thread>

#include "core/common.hh"
#include "core/dsp/audio_buffer.hh"
#include "core/dsp/midi_stack.hh"

namespace soir {
namespace dsp {

class MidiExt {
 public:
  MidiExt();
  ~MidiExt();

  absl::Status Init(const std::string& settings);
  absl::Status Start();
  absl::Status Run();
  absl::Status Stop();

  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer);

 private:
  void ScheduleMidiEvents(const absl::Time& next_block_at);
  void FillAudioBuffer(Uint8* stream, int len);
  void WaitForInitialTick();

  // Current configuration as set from live coding.
  std::string current_settings_ = "";
  int current_midi_port_ = -1;
  int current_audio_device_ = -1;

  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread thread_;
  bool stop_ = false;
  std::vector<Uint8> consumed_;
  std::list<AudioBuffer> buffers_;
  SampleTick current_tick_ = 0;

  // Underlying devices, use current_* to know if they are valid and
  // can be accessed or not.
  libremidi::midi_out midiout_;
  SDL_AudioDeviceID audio_device_id_ = 0;

  MidiStack midi_stack_;
};

}  // namespace dsp
}  // namespace soir
