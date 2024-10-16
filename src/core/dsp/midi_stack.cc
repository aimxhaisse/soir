#include <absl/log/log.h>

#include "core/dsp/midi_stack.hh"

namespace neon {
namespace dsp {

MidiStack::MidiStack() {}

void MidiStack::AddEvents(const std::list<MidiEventAt>& events) {
  for (auto event : events) {
    auto it = sorted_events_.begin();

    while (it != sorted_events_.end() && it->Tick() < event.Tick()) {
      ++it;
    }

    sorted_events_.insert(it, event);
  }
}

void MidiStack::EventsAtTick(SampleTick sample,
                             std::list<MidiEventAt>& events) {
  events.clear();

  if (sorted_events_.empty()) {
    return;
  }

  auto it = sorted_events_.begin();
  if (it->Tick() > sample) {
    return;
  }

  while (it != sorted_events_.end() && it->Tick() <= sample) {
    events.push_back(*it);
    it = sorted_events_.erase(it);
  }
}

}  // namespace dsp
}  // namespace neon
