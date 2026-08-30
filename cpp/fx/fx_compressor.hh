#pragma once

#include <absl/status/status.h>
#include <absl/time/time.h>

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
  void WarnSourceMissing();

  Controls* controls_;
  SignalBus* bus_;

  std::mutex mutex_;
  Fx::Settings settings_;

  // The sidechain source: "self", "master", or a track name.
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

  // Rate-limited "source missing" warning state.
  bool warned_missing_ = false;
  absl::Time last_missing_warn_{};
};

}  // namespace fx
}  // namespace soir
