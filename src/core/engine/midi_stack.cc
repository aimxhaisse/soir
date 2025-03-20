#include <absl/log/log.h>

#include "core/engine/midi_stack.hh"

namespace soir {
namespace engine {

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
  auto it = sorted_events_.begin();
  while (it != sorted_events_.end() && it->Tick() <= sample) {
    events.push_back(*it);
    it = sorted_events_.erase(it);
  }
}

}  // namespace engine
}  // namespace soir
