#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <list>
#include <map>
#include <memory>

#include "core/dsp/adsr.hh"
#include "core/dsp/audio_buffer.hh"
#include "core/dsp/dsp.hh"
#include "core/dsp/track.hh"
#include "utils/config.hh"

namespace neon {
namespace dsp {

struct TrackSettings;

class MonoSampler {
 public:
  MonoSampler() = default;
  ~MonoSampler() = default;

  absl::Status Init(const TrackSettings& settings);
  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  // We only support MONO audio for now.
  struct Sampler {
    void NoteOn();
    void NoteOff();

    bool is_playing_;
    int pos_;
    std::vector<float> buffer_;

    // This ADSR envelope is used to avoid glitches at the beginning
    // and at the end of samples, which is utils with raw data where
    // there is directly something starting with a non-zero value in
    // the first or the last sample.
    ADSR wrapper_;
  };

  std::string directory_;
  std::map<int, std::unique_ptr<Sampler>> samplers_;
  std::set<Sampler*> playing_;
};

}  // namespace dsp
}  // namespace neon
