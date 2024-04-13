#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <mutex>
#include <optional>

#include "core/dsp/audio_buffer.hh"
#include "core/dsp/mono_sampler.hh"
#include "utils/config.hh"

namespace neon {
namespace dsp {

class MonoSampler;

// Types of instruments available.
typedef enum {
  TRACK_MONO_SAMPLER = 0,
} TrackInstrument;

// Settings of a track.
struct TrackSettings {
  TrackInstrument instrument_;
  int channel_ = 0;
  bool muted_ = false;
  int volume_ = 127;
  int pan_ = 64;
};

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  Track();

  absl::Status Init(const TrackSettings& settings);

  // If MaybeFastUpdate returns false, it means the track can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const TrackSettings& settings);
  void FastUpdate(const TrackSettings& settings);

  TrackInstrument GetInstrument();
  TrackSettings GetSettings();
  int GetChannel();

  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  void HandleMidiEvent(const libremidi::message& event);

  std::mutex mutex_;
  TrackSettings settings_;
  std::unique_ptr<MonoSampler> sampler_;
};

}  // namespace dsp
}  // namespace neon
