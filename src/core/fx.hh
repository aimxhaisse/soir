#pragma once

#include <absl/status/status.h>
#include <mutex>

#include "core/common.hh"
#include "core/audio_buffer.hh"

namespace soir {


// Similar to tracks, can be called from two contexts:
//
// - Rt context to update parameters of the Fx,
// - Dsp context to actually render the track,
//
// Those two contexts are protected by mutex_.
struct Fx {
  // Types of instruments available.
  typedef enum { FX_UNKNOWN = 0, FX_REVERB = 1, FX_CHORUS = 2 } FxType;

  // Settings of an Fx.
  struct Settings {
    std::string name_ = "unknown";
    FxType type_ = FX_UNKNOWN;
    float mix_ = 0.0;
    std::string extra_;
  };

  virtual ~Fx() {};

  virtual absl::Status Init(const Settings& settings) = 0;

  // If MaybeFastUpdate returns false, it means the FX can't update
  // itself quickly so it likely needs to be re-created.

  virtual bool CanFastUpdate(const Settings& settings) = 0;
  virtual void FastUpdate(const Settings& settings) = 0;
  virtual void Render(SampleTick tick, AudioBuffer&) = 0;
};


}  // namespace soir
