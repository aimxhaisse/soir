#pragma once

#include <absl/status/status.h>

#include <list>
#include <string>

#include "audio/audio_buffer.hh"
#include "core/common.hh"
#include "core/midi_event.hh"

namespace soir {
namespace fx {

enum class Type { UNKNOWN, CHORUS, REVERB, LPF, HPF, ECHO, VST };

struct Fx {
  struct Settings {
    std::string name_;
    std::string extra_;
    Type type_;
    float mix_ = 0.0;
  };

  virtual ~Fx() = default;
  virtual absl::Status Init(const Settings& settings) = 0;
  virtual bool CanFastUpdate(const Settings& settings) = 0;
  virtual void FastUpdate(const Settings& settings) = 0;
  virtual void Render(SampleTick tick, AudioBuffer& buffer,
                      const std::list<MidiEventAt>& events) = 0;
};

}  // namespace fx
}  // namespace soir
