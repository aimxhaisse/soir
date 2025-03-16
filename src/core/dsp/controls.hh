#pragma once

#include <absl/status/status.h>
#include <map>
#include <memory>
#include <shared_mutex>

#include "core/common.hh"
#include "core/dsp/dsp.hh"
#include "core/dsp/midi_stack.hh"

namespace soir {
namespace dsp {

// A control that is interpolated over time.
class Control {
 public:
  Control();

  // This is meant to be used by the RT thread to update the target
  // value of the knob against which we interpolate.
  void SetTargetValue(SampleTick tick, float target);

  // Returns the interpolated value at the given tick. This takes a
  // shared lock on each read:
  //
  // --> 48 000 * number of parameters * numbers of usages
  //
  // This may be heavy, let's maybe benchmark this at some point and
  // maybe consider an atomic alternative if it's too slow or some
  // other trick.
  float GetValue(SampleTick tick);

 private:
  std::shared_mutex mutex_;

  SampleTick fromTick_ = 0;
  SampleTick toTick_ = 0;

  float initialValue_ = 0.0f;
  float targetValue_ = 0.0f;
};

// A collection of controls that can be used to control the DSP.
class Controls {
 public:
  Controls();

  absl::Status Init();

  void AddEvents(const std::list<MidiEventAt>& events);
  void Update(SampleTick current);
  std::shared_ptr<Control> GetControl(const std::string& name);

 private:
  void ProcessEvent(MidiEventAt& event_at);

  std::shared_mutex mutex_;
  std::map<std::string, std::shared_ptr<Control>> controls_;
  MidiStack midi_stack_;
};

}  // namespace dsp
}  // namespace soir
