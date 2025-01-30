#pragma once

#include <absl/status/status.h>
#include <vector>

#include "core/dsp/audio_buffer.hh"

namespace soir {
namespace dsp {

static constexpr int kNumChannels = 2;
static constexpr int kLeftChannel = 0;
static constexpr int kRightChannel = 1;
static constexpr int kSampleRate = 48000;
static constexpr float kVorbisQuality = 1.0f;

// This is static for now and mostly based on my local setup, we might
// want to make this configurable in the future though it implies careful
// considerations.

// Size of a processing block (~10ms). This is also the resolution at
// which we perform control parameter updates (100 times per second),
// we assume it's not hearable below this. This is also the resolution
// of external device MIDI scheduling.
static constexpr int kBlockSize = 512;

// Resolution of MIDI event scheduling, independant of block size so
// that we can increase block size without affecting scheduling.
static constexpr int kMidiExtChunkSize = 128;

// Number of blocks between scheduling and actual processing (~70ms),
// this is in case we have heavy processing in the code loops. This number
// *needs* to be higher than the kMidiDeviceDelay parameter, which schedules
// a bit in the past MIDI events so that when capturing them back on the
// audio device we get something accurate.
static constexpr int kBlockProcessingDelay = 7;

class SampleConsumer {
 public:
  virtual absl::Status PushAudioBuffer(AudioBuffer&) = 0;
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
}  // namespace soir
