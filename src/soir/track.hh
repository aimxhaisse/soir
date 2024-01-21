#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <mutex>

#include "audio_buffer.hh"
#include "common/config.hh"
#include "live.grpc.pb.h"
#include "mono_sampler.hh"

namespace maethstro {
namespace soir {

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  Track();

  absl::Status Init(const proto::Track& config);

  // If MaybeFastUpdate returns false, it means the track can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const proto::Track& config);
  void FastUpdate(const proto::Track& config);

  proto::Track::Instrument GetInstrument();
  int GetChannel();
  int GetVolume();
  int GetPan();
  bool IsMuted();

  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  void HandleMidiEvent(const libremidi::message& event);

  std::mutex mutex_;
  proto::Track::Instrument instrument_;
  int channel_ = 0;
  bool muted_ = false;
  int volume_ = 127;
  int pan_ = 64;

  std::unique_ptr<MonoSampler> sampler_;
};

}  // namespace soir
}  // namespace maethstro
