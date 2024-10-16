#pragma once

#include <list>

#include "core/common.hh"

namespace neon {
namespace dsp {

class MidiStack {
 public:
  MidiStack();

  void AddEvents(const std::list<MidiEventAt>& events);
  void EventsAtTick(SampleTick sample, std::list<MidiEventAt>& events);

 private:
  std::list<MidiEventAt> sorted_events_;
};

}  // namespace dsp
}  // namespace neon
