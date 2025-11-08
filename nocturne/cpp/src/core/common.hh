#pragma once

#include <string_view>

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
#define SOIR_TRACING_ZONE_COLOR(name, color)
#define SOIR_TRACING_FRAME(name)
#endif  // SOIR_ENABLE_TRACY

namespace soir {

// Type of a sample in the audio engine, there are ~48000 ticks per
// second so we need to use a 64 bits to prevent near overflows.
using SampleTick = uint64_t;

// This is the name of the track used by the RT engine to communicate
// with the DSP engine for MIDI events that update the controls via
// interpolation. Both side have to agree on this name so it is
// defined here.
static constexpr std::string_view kInternalControls = "soir_internal_controls";

}  // namespace soir
