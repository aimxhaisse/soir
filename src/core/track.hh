#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <map>
#include <mutex>
#include <optional>

#include "core/audio_buffer.hh"
#include "core/controls.hh"
#include "core/fx/fx_stack.hh"
#include "core/midi_ext.hh"
#include "core/midi_stack.hh"
#include "core/parameter.hh"
#include "core/sample_manager.hh"
#include "core/sampler.hh"
#include "utils/config.hh"

namespace soir {

class Sampler;

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  // Types of instruments available.
  typedef enum {
    TRACK_UNKNOWN = 0,
    TRACK_SAMPLER = 1,
    TRACK_MIDI_EXT = 2,
  } TrackInstrument;

  // Settings of a track.
  struct Settings {
    std::string name_ = "unknown";
    TrackInstrument instrument_ = TRACK_UNKNOWN;
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

  TrackInstrument GetInstrument();
  Settings GetSettings();
  const std::string& GetTrackName();

  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);

 private:
  Controls* controls_;

  std::mutex mutex_;
  Settings settings_;
  std::unique_ptr<Sampler> sampler_;
  std::unique_ptr<MidiExt> midi_ext_;
  std::unique_ptr<fx::FxStack> fx_stack_;
  MidiStack midi_stack_;
};

}  // namespace soir
