#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <memory>

#include "audio_buffer.hh"
#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace soir {

class MonoSampler {
 public:
  MonoSampler() = default;
  ~MonoSampler() = default;

  absl::Status Init(const common::Config& config, int channel);
  void Render(const std::list<proto::MidiEvents_Request>&, AudioBuffer&);

 private:
  // We only support MONO audio for now.
  struct Sampler {
    bool is_playing_;
    int pos_;
    std::vector<float> buffer_;
  };

  int channel_ = 0;
  std::string directory_;
  std::map<int, std::unique_ptr<Sampler>> samplers_;
};

}  // namespace soir
}  // namespace maethstro
