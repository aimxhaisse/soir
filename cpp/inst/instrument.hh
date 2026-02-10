#pragma once

#include <absl/status/status.h>

#include <list>

#include "core/common.hh"
#include "core/midi_event.hh"

namespace soir {

class SampleManager;
class Controls;

namespace inst {

enum class Type { UNKNOWN, SAMPLER, EXTERNAL, VST };

// Abstract class for instruments. Some things may not be needed
// for all instrumnets (such as the sample manager for instance), we might
// move this in a "context" class or something containing different pieces of
// the engine.
class Instrument {
 public:
  virtual ~Instrument() = default;
  virtual absl::Status Init(const std::string& settings,
                            SampleManager* sample_manager,
                            Controls* controls) = 0;
  virtual void Render(SampleTick, const std::list<MidiEventAt>&,
                      AudioBuffer&) = 0;
  virtual Type GetType() const = 0;
  virtual std::string GetName() const = 0;

  // Only needed if the instrument requires a thread, thus a default
  // empty implementation is provided.
  virtual absl::Status Start() { return absl::OkStatus(); }
  virtual absl::Status Run() { return absl::OkStatus(); }
  virtual absl::Status Stop() { return absl::OkStatus(); }
};

}  // namespace inst
}  // namespace soir
