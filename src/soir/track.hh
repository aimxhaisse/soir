#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>

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

  absl::Status Init(const common::Config& config);
  int GetChannel() const { return channel_; }

  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  std::unique_ptr<MonoSampler> sampler_;
  int channel_ = 0;
};

}  // namespace soir
}  // namespace maethstro
