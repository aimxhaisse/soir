#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <list>
#include <map>
#include <memory>

#include "core/dsp/adsr.hh"
#include "core/dsp/audio_buffer.hh"
#include "core/dsp/dsp.hh"
#include "core/dsp/sample_manager.hh"
#include "core/dsp/sample_pack.hh"
#include "core/dsp/track.hh"
#include "neon.grpc.pb.h"
#include "utils/config.hh"

namespace neon {
namespace dsp {

struct TrackSettings;

class MonoSampler {
 public:
  absl::Status Init(SampleManager* sample_manager);
  void Render(const std::list<libremidi::message>&, AudioBuffer&);

 private:
  void HandleSysex(const proto::MidiSysexInstruction& sysex);

  void PlaySample(Sample* s);
  void StopSample(Sample* s);
  Sample* GetSample(const std::string& pack, const std::string& name);

  // We only support MONO audio for now.
  struct PlayingSample {
    int pos_;
    Sample* sample_;

    // This ADSR envelope is used to avoid glitches at the beginning
    // and at the end of samples, which is utils with raw data where
    // there is directly something starting with a non-zero value in
    // the first or the last sample.
    ADSR wrapper_;
  };

  // We handle playing multiple times the same sample, we can remove
  // then from the list when they are done or in a FIFO mode if the
  // user triggers a midi note off.
  std::map<Sample*, std::list<std::unique_ptr<PlayingSample>>> playing_;
  SampleManager* sample_manager_;
};

}  // namespace dsp
}  // namespace neon
