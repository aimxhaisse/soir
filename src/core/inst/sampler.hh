#pragma once

#include <absl/status/status.h>
#include <rapidjson/document.h>
#include <libremidi/libremidi.hpp>
#include <list>
#include <map>
#include <memory>

#include "core/adsr.hh"
#include "core/audio_buffer.hh"
#include "core/common.hh"
#include "core/controls.hh"
#include "core/dsp.hh"
#include "core/inst/instrument.hh"
#include "core/midi_stack.hh"
#include "core/parameter.hh"
#include "core/sample_manager.hh"
#include "core/sample_pack.hh"
#include "soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {
namespace inst {

// This is to prevent clipping when we play a sample that doesn't
// start with an amp of 0 or that we need to suddenly cut without
// going through the envelope. We ensure there is a very small attack
// and release no matter what.
static constexpr float kSampleMinimalSmoothingMs = 1.0f;
static constexpr float kSampleMinimalDurationMs = 2 * kSampleMinimalSmoothingMs;
static constexpr int kSampleMinimalSmoothingSamples =
    kSampleMinimalDurationMs * kSampleRate / 1000;

// This is the main class that will handle the rendering of the samples
class Sampler : public Instrument {
 public:
  absl::Status Init(const std::string& settings, SampleManager* sample_manager,
                    Controls* controls);
  void Render(SampleTick tick, const std::list<MidiEventAt>&, AudioBuffer&);
  Type GetType() const { return Type::SAMPLER; }
  std::string GetName() const { return "Sampler"; }

 private:
  void ProcessMidiEvents(SampleTick tick);
  void HandleSysex(const proto::MidiSysexInstruction& sysex);

  // Parameters for PlaySample
  struct PlaySampleParameters {
    float start_ = 0.0f;
    float end_ = 1.0f;
    Parameter pan_;
    float rate_ = 1.0f;
    float attack_ = 0.0f;
    float decay_ = 0.0f;
    float level_ = 1.0f;
    float release_ = 0.0f;
    Parameter amp_ = 1.0f;

    static void FromJson(Controls* controls, const rapidjson::Value& v,
                         PlaySampleParameters* p);
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
    Parameter pan_;
    float rate_;
    Parameter amp_;

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
  Controls* controls_;
  MidiStack midi_stack_;
};

}  // namespace inst
}  // namespace soir
