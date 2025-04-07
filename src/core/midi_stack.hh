#pragma once

#include <list>

#include "core/common.hh"
#include "core/midi_event.hh"

namespace soir {

class MidiStack {
 public:
  MidiStack();

  void AddEvents(const std::list<MidiEventAt>& events);
  void EventsAtTick(SampleTick sample, std::list<MidiEventAt>& events);

 private:
  std::list<MidiEventAt> sorted_events_;
};

}  // namespace soir
