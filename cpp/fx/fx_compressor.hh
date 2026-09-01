#pragma once

#include <absl/status/status.h>

#include <array>
#include <mutex>
#include <string>

#include "core/common.hh"
#include "core/parameter.hh"
#include "core/signal_bus.hh"
#include "dsp/compressor.hh"
#include "fx.hh"

namespace soir {
namespace fx {

// Compressor effect.
//
// The gain reduction can be computed from the compressor's own input
// (source "self", plain in-line compression), from the master mix
// (source "master"), or from the rendered output of another track
// (source = track name, sidechain compression). External sources are
// read from the SignalBus with a deterministic one-block delay.
struct Compressor : public Fx {
  Compressor(Controls* controls, SignalBus* bus);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer& buffer,
              const std::list<MidiEventAt>& events) override;

 private:
  void ReloadParams();
  void ResolveSource();
  void WarnSourceMissing();

  Controls* controls_;
  SignalBus* bus_;

  std::mutex mutex_;
  Fx::Settings settings_;

  // The sidechain source: "self", "master", or a track name.
  std::string source_ = "self";
  // Stable bus handle of the source signal, nullptr while unknown.
  SignalBus::Signal* source_signal_ = nullptr;

  Parameter threshold_;
  Parameter ratio_;
  Parameter attack_;
  Parameter release_;
  Parameter knee_;
  Parameter makeup_;
  Parameter wet_;

  dsp::Compressor::Parameters params_;
  dsp::Compressor comp_;

  // Scratch buffers for the sidechain key: Latest() copies the source
  // block into them each block, so the render loop never reads the
  // bus ring directly.
  std::array<float, kBlockSize> src_left_;
  std::array<float, kBlockSize> src_right_;

  // Set once the missing-source warning has been emitted since the
  // last time the source was found: the warning is logged on the
  // transition into "missing" only, never repeatedly from the audio
  // path.
  bool warned_missing_ = false;
};

}  // namespace fx
}  // namespace soir
