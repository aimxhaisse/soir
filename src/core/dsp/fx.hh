#pragma once

#include <absl/status/status.h>
#include <mutex>

#include "core/common.hh"
#include "core/dsp/audio_buffer.hh"

namespace soir {
namespace dsp {

// Types of instruments available.
typedef enum { FX_REVERB = 0, FX_CHORUS = 1 } FxType;

// Settings of an Fx.
struct FxSettings {
  std::string name_ = "unknown";
  FxType fx_ = FX_REVERB;
  float mix_ = 0.0;
  std::string extra_;
};

struct Fx {
  Fx();

  absl::Status Init(const FxSettings& settings);
  absl::Status Stop();

  // If MaybeFastUpdate returns false, it means the FX can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const FxSettings& settings);
  void FastUpdate(const FxSettings& settings);
  void Render(SampleTick tick, AudioBuffer&);

 private:
  std::mutex mutex_;
  FxSettings settings_;
};

}  // namespace dsp
}  // namespace soir
