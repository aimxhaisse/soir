#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <mutex>

#include "core/common.hh"
#include "core/dsp/audio_buffer.hh"
#include "core/dsp/fx.hh"

namespace soir {
namespace dsp {

// Represents a stack of ordered DSP fx.
class FxStack {
 public:
  FxStack();

  absl::Status Init(const std::list<FxSettings> fx_settings);
  absl::Status Stop();

  bool CanFastUpdate(const std::list<FxSettings> fx_settings);
  void FastUpdate(const std::list<FxSettings> fx_settings);
  void Render(SampleTick tick, AudioBuffer&);

 private:
  std::mutex mutex_;
  std::list<std::string> order_;
  std::map<std::string, std::unique_ptr<Fx>> fxs_;
};

}  // namespace dsp
}  // namespace soir
