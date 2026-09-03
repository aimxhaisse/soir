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
// source: "self" (in-line, sample-accurate), "master", or a track
// name (sidechain, one-block delay via the SignalBus).
struct Compressor : public Fx {
  Compressor(Controls* controls, SignalBus* bus);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer& buffer,
              const std::list<MidiEventAt>& events) override;

 private:
  void ReloadParams();
  void HandleMissingSource(const std::string& name, SampleTick tick);

  Controls* controls_;
  SignalBus* bus_;

  std::mutex mutex_;
  Fx::Settings settings_;

  // The key source: "self", "master", or a track name. Looked up by
  // name in Render(); a source created after this FX is picked up
  // from its first published block.
  std::string source_ = "self";

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

  // Latched per missing-source transition, cleared on the next
  // successful read.
  bool warned_missing_ = false;
  // Set once the source delivered a block: separates ramp-up (quiet)
  // from a stopped source (warns).
  bool had_source_ = false;
};

}  // namespace fx
}  // namespace soir
