#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
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

  absl::Status Init(const proto::Track& config);
  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  // We only support MONO audio for now.
  struct Sampler {
    void NoteOn();
    void NoteOff();

    bool is_playing_;
    int pos_;
    std::vector<float> buffer_;
  };

  std::string directory_;
  std::map<int, std::unique_ptr<Sampler>> samplers_;
};

}  // namespace soir
}  // namespace maethstro
