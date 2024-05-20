#pragma once

#include <absl/status/status.h>
#include <list>

#include <libremidi/libremidi.hpp>

#include "core/dsp/audio_buffer.hh"

namespace neon {
namespace dsp {

class MidiExt {
 public:
  MidiExt();
  ~MidiExt();

  absl::Status Init();
  void Render(const std::list<libremidi::message>& events, AudioBuffer& buffer);

 private:
  libremidi::midi_out midiout_;
};

}  // namespace dsp
}  // namespace neon
