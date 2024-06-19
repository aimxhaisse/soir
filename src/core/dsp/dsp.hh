#pragma once

#include <absl/status/status.h>
#include <vector>

#include "core/dsp/audio_buffer.hh"

namespace neon {
namespace dsp {

static constexpr int kNumChannels = 2;
static constexpr int kLeftChannel = 0;
static constexpr int kRightChannel = 1;
static constexpr int kSampleRate = 48000;
static constexpr float kVorbisQuality = 1.0f;
static constexpr int kDeviceId = 0;
static constexpr int kNumBuffers = 8;

class SampleConsumer {
 public:
  virtual absl::Status PushAudioBuffer(const AudioBuffer&) = 0;
};

// This is not standard MIDI, will likely evolve if we want to
// natively support some controllers without doing any work.  For node
// MidiMIX provides a way to map any CC to knobs so we can provide a
// working mapping for it.
static constexpr int kMidiControlMuteTrack = 0x01;
static constexpr int kMidiControlVolume = 0x02;
static constexpr int kMidiControlPan = 0x03;
static constexpr int kMidiControlFilter = 0x04;
static constexpr int kMidiControlReverb = 0x05;

static constexpr bool kTrackDefaultMuted = false;
static constexpr int kTrackDefaultVolume = 127;
static constexpr int kTrackDefaultPan = 64;

}  // namespace dsp
}  // namespace neon
