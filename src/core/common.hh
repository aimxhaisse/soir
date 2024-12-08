#pragma once

#include <absl/time/clock.h>
#include <libremidi/libremidi.hpp>

namespace soir {

using SampleTick = uint64_t;
using MicroBeat = uint64_t;
static constexpr uint64_t kOneBeat = 1000000;

// Goal of this class is to store a midi event with a timestamp and
// map it to a tick on the rendering side.
class MidiEventAt {
 public:
  MidiEventAt(int track, const libremidi::message& msg, absl::Time at)
      : track_(track), msg_(msg), at_(at), tick_(0) {}

  int Track() const { return track_; }
  const libremidi::message& Msg() const { return msg_; }
  const absl::Time At() const { return at_; }
  void SetTick(SampleTick tick) { tick_ = tick; }
  SampleTick Tick() const { return tick_; }

 private:
  // Track number of the event, this is used to route the event to the
  // correct track in the DSP. A track can control multiple MIDI
  // channels and is independent.
  int track_;

  libremidi::message msg_;

  // This is set in a first time at the creation of the event, the
  // goal is to have something as close as possible as the live coding
  // side. We add a small delay to it so that the processing/context
  // switchs with locking on the DSP side are negated.
  absl::Time at_;

  // This is set in a second time after the event is scheduled, goal
  // is to be as close as possible to the actual time the event is
  // played. We set this via SetTick in the DSP rendering loop.
  SampleTick tick_;
};

}  // namespace soir
