#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <mutex>

#include "core/common.hh"
#include "core/audio_buffer.hh"
#include "core/fx.hh"

namespace soir {


// Represents a stack of ordered DSP fx.
class FxStack {
 public:
  FxStack(Controls* controls);

  absl::Status Init(const std::list<Fx::Settings> fx_settings);

  // This is not the most optimal implementation: if an FX is added to
  // the list we consider we can't update it quickly (while we could
  // do a two stages init with new allocation outside the DSP path).
  //
  // It's simple enough for now though.
  bool CanFastUpdate(const std::list<Fx::Settings> fx_settings);
  void FastUpdate(const std::list<Fx::Settings> fx_settings);
  void Render(SampleTick tick, AudioBuffer& buffer);

 private:
  Controls* controls_;

  std::mutex mutex_;
  std::list<std::string> order_;
  std::map<std::string, std::unique_ptr<Fx>> fxs_;
};


}  // namespace soir
