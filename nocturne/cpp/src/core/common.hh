#pragma once

#include <absl/status/status.h>

#include <string_view>

namespace soir {
class AudioBuffer;
}

// Used for debugging mainly (Tracy colors)
#define SOIR_BLUE 0x00A5E3
#define SOIR_GREEN 0x8DD7BF
#define SOIR_RED 0xFF5768
#define SOIR_PINK 0xFF96C5
#define SOIR_ORANGE 0xFFBF65

// Enable Tracy for profiling
#ifdef SOIR_ENABLE_TRACING
#include "tracy/Tracy.hpp"
#define SOIR_TRACING_ZONE(name) ZoneScopedN(name)
#define SOIR_TRACING_ZONE_STR(name) \
  ZoneScopedN;                      \
  ZoneName(name.c_str(), name.size())
#define SOIR_TRACING_ZONE_COLOR(name, color) ZoneScopedNC(name, color)
#define SOIR_TRACING_ZONE_COLOR_STR(name, color) \
  ZoneScoped;                                    \
  ZoneName(name.c_str(), name.size());           \
  ZoneColor(color)
#define SOIR_TRACING_FRAME(name) FrameMarkNamed(name)
#else
#define SOIR_TRACING_ZONE(name)
#define SOIR_TRACING_ZONE_STR(name)
#define SOIR_TRACING_ZONE_COLOR(name, color)
#define SOIR_TRACING_ZONE_COLOR_STR(name, color)
#define SOIR_TRACING_FRAME(name)
#endif  // SOIR_ENABLE_TRACING

namespace soir {

// Type of a sample in the audio engine, there are ~48000 ticks per
// second so we need to use a 64 bits to prevent near overflows.
using SampleTick = uint64_t;

// This is the name of the track used by the RT engine to communicate
// with the DSP engine for MIDI events that update the controls via
// interpolation. Both side have to agree on this name so it is
// defined here.
static constexpr std::string_view kInternalControls = "soir_internal_controls";

// Audio constants
static constexpr int kSampleRate = 48000;
static constexpr int kNumChannels = 2;
static constexpr int kLeftChannel = 0;
static constexpr int kRightChannel = 1;
static constexpr float kPI = 3.14159265358979323846f;
static constexpr float kVorbisQuality = 1.0f;

// Processing block and timing constants
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

// Frequency bounds
// Hz (lower bound of human hearing)
static constexpr float kMinFreq = 20.0f;

// Hz (upper bound of human hearing)
static constexpr float kMaxFreq = 20000.0f;

// Number of times per second the control knobs are updated. This is
// way lower than the sample frequency because values are computed
// from Python so it is slow (but flexible). We interpolate values in
// between.
static constexpr int kControlsFrequencyUpdate = 100;

// MIDI control constants
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

// Interface for components that consume audio buffers (audio output, recorders,
// etc.)
class SampleConsumer {
 public:
  virtual ~SampleConsumer() = default;
  virtual absl::Status PushAudioBuffer(AudioBuffer&) = 0;
};

}  // namespace soir
