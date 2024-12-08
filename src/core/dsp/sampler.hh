#pragma once

#include <absl/status/status.h>
#include <libremidi/libremidi.hpp>
#include <list>
#include <map>
#include <memory>

#include "core/common.hh"
#include "core/dsp/adsr.hh"
#include "core/dsp/audio_buffer.hh"
#include "core/dsp/dsp.hh"
#include "core/dsp/midi_stack.hh"
#include "core/dsp/sample_manager.hh"
#include "core/dsp/sample_pack.hh"
#include "core/dsp/track.hh"
#include "proto/soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {
namespace dsp {

struct TrackSettings;

// This is to prevent clipping when we play a sample that doesn't
// start with an amp of 0 or that we need to suddenly cut without
// going through the envelope. We ensure there is a very small attack
// and release no matter what.
static constexpr float kSampleMinimalSmoothingMs = 1.0f;
static constexpr float kSampleMinimalDurationMs = 2 * kSampleMinimalSmoothingMs;
static constexpr int kSampleMinimalSmoothingSamples =
    kSampleMinimalDurationMs * kSampleRate / 1000;

class Sampler {
 public:
  absl::Status Init(SampleManager* sample_manager);
  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);

 private:
  void ProcessMidiEvents(SampleTick tick);
  void HandleSysex(const proto::MidiSysexInstruction& sysex);

  void PlaySample(Sample* s, float start, float end);
  void StopSample(Sample* s);
  Sample* GetSample(const std::string& pack, const std::string& name);

  // We only support MONO audio for now.
  struct PlayingSample {
    int pos_;
    Sample* sample_;
    bool removing_ = false;

    int start_;
    int end_;
    int inc_;

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

  MidiStack midi_stack_;
};

}  // namespace dsp
}  // namespace soir
