#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <map>
#include <mutex>
#include <optional>

#include "core/dsp/audio_buffer.hh"
#include "core/dsp/fx_stack.hh"
#include "core/dsp/midi_ext.hh"
#include "core/dsp/midi_stack.hh"
#include "core/dsp/sample_manager.hh"
#include "core/dsp/sampler.hh"
#include "utils/config.hh"

namespace soir {
namespace dsp {

class Sampler;

// Types of instruments available.
typedef enum {
  TRACK_SAMPLER = 0,
  TRACK_MIDI_EXT = 1,
} TrackInstrument;

// Settings of a track.
struct TrackSettings {
  TrackInstrument instrument_;
  std::string name_ = "unknown";
  bool muted_ = false;
  int volume_ = 127;
  int pan_ = 64;
  std::list<FxSettings> fxs_;
  std::string extra_;
};

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  Track();

  absl::Status Init(const TrackSettings& settings,
                    SampleManager* sample_manager);
  absl::Status Stop();

  // If MaybeFastUpdate returns false, it means the track can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const TrackSettings& settings);
  void FastUpdate(const TrackSettings& settings);

  TrackInstrument GetInstrument();
  TrackSettings GetSettings();
  const std::string& GetTrackName();

  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);

 private:
  std::mutex mutex_;
  TrackSettings settings_;
  std::unique_ptr<Sampler> sampler_;
  std::unique_ptr<MidiExt> midi_ext_;
  FxStack fx_stack_;
  MidiStack midi_stack_;
};

}  // namespace dsp
}  // namespace soir
