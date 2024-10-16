#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <list>

#include "core/dsp/audio_buffer.hh"
#include "core/dsp/midi_stack.hh"

namespace neon {
namespace dsp {

class MidiExt {
 public:
  MidiExt();
  ~MidiExt();

  absl::Status Init();
  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer);

 private:
  MidiStack midi_stack_;
  libremidi::midi_out midiout_;
};

}  // namespace dsp
}  // namespace neon
