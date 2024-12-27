#pragma once

#include <absl/status/status.h>
#include <rapidjson/document.h>
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
#include "soir.grpc.pb.h"
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

// This is the main class that will handle the rendering of the samples
class Sampler {
 public:
  absl::Status Init(SampleManager* sample_manager);
  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);

 private:
  void ProcessMidiEvents(SampleTick tick);
  void HandleSysex(const proto::MidiSysexInstruction& sysex);

  // Parameters for PlaySample
  struct PlaySampleParameters {
    float start_ = 0.0f;
    float end_ = 1.0f;
    float pan_ = 0.0f;
    float rate_ = 1.0f;
    float attack_ = 0.0f;
    float decay_ = 0.0f;
    float sustain_ = 1.0f;
    float release_ = 0.0f;
    float amp_ = 1.0f;

    static void FromJson(const rapidjson::Value& v, PlaySampleParameters* p);
  };

  void PlaySample(Sample* sp, const PlaySampleParameters& p);
  void StopSample(Sample* s);
  Sample* GetSample(const std::string& pack, const std::string& name);
  float Interpolate(const std::vector<float>& v, float pos);

  // We only support MONO audio for now.
  struct PlayingSample {
    float pos_;
    Sample* sample_;
    bool removing_ = false;

    int start_;
    int end_;
    int inc_;
    float pan_;
    float rate_;
    float amp_;

    // This ADSR envelope is used to avoid glitches at the beginning
    // and at the end of samples, which is utils with raw data where
    // there is directly something starting with a non-zero value in
    // the first or the last sample.
    ADSR wrapper_;

    // This ADSR envelope is on top of the previous one and is
    // controlled by the live code.
    ADSR env_;
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
