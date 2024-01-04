#pragma once

#include <absl/status/status.h>

#include "audio_buffer.hh"
#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace soir {

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  Track();

  absl::Status Init(const common::Config& config);

  void Render(const std::list<proto::MidiEvents_Request>&, AudioBuffer&);

 private:
  // We only support MONO audio for now.
  struct MonoSampler {
    bool is_playing_;
    int pos_;
    std::vector<float> buffer_;
  };

  int channel_ = 0;
  std::map<int, std::unique_ptr<MonoSampler>> samples_;
};

}  // namespace soir
}  // namespace maethstro
