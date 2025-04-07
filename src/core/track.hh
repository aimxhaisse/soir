#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <map>
#include <mutex>
#include <optional>

#include "core/audio_buffer.hh"
#include "core/common.hh"
#include "core/controls.hh"
#include "core/fx/fx_stack.hh"
#include "core/inst/instrument.hh"
#include "core/inst/midi_ext.hh"
#include "core/inst/sampler.hh"
#include "core/midi_stack.hh"
#include "core/parameter.hh"
#include "core/sample_manager.hh"
#include "utils/config.hh"

namespace soir {

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  // Settings of a track.
  struct Settings {
    std::string name_ = "unknown";
    inst::Type instrument_ = inst::Type::UNKNOWN;
    bool muted_ = false;
    Parameter volume_;
    Parameter pan_;
    std::string extra_;
    std::list<fx::Fx::Settings> fxs_;
  };

  Track();

  absl::Status Init(const Settings& settings, SampleManager* sample_manager,
                    Controls* controls);
  absl::Status Stop();

  // If MaybeFastUpdate returns false, it means the track can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const Settings& settings);
  void FastUpdate(const Settings& settings);

  Settings GetSettings();
  const std::string& GetTrackName();

  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);

 private:
  Controls* controls_;
  SampleManager* sample_manager_;

  std::mutex mutex_;
  Settings settings_;
  std::unique_ptr<inst::Instrument> inst_;
  std::unique_ptr<fx::FxStack> fx_stack_;
  MidiStack midi_stack_;
};

}  // namespace soir
