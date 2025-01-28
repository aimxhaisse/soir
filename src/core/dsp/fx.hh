#pragma once

#include <absl/status/status.h>
#include <mutex>

#include "core/common.hh"
#include "core/dsp/audio_buffer.hh"

namespace soir {
namespace dsp {

class FxChorus;

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

  Fx();

  absl::Status Init(const Settings& settings);
  absl::Status Stop();

  // If MaybeFastUpdate returns false, it means the FX can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const Settings& settings);
  void FastUpdate(const Settings& settings);
  void Render(SampleTick tick, AudioBuffer&);

 private:
  std::mutex mutex_;
  Settings settings_;
};

}  // namespace dsp
}  // namespace soir
