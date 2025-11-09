#pragma once

#include <string>

#include <absl/status/status.h>

#include "audio/audio_buffer.hh"
#include "core/common.hh"

namespace soir {
namespace fx {

enum class Type { UNKNOWN, CHORUS, REVERB, LPF, HPF, ECHO };

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
  virtual void Render(SampleTick tick, AudioBuffer& buffer) = 0;
};

}  // namespace fx
}  // namespace soir
