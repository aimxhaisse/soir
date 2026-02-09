#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <list>
#include <map>
#include <mutex>

#include "audio/audio_buffer.hh"
#include "core/common.hh"
#include "core/parameter.hh"
#include "fx.hh"

namespace soir {
namespace vst {
class VstHost;
}  // namespace vst

namespace fx {

struct FxVst;
class FxStack {
 public:
  FxStack(Controls* controls, vst::VstHost* vst_host);

  absl::Status Init(const std::list<Fx::Settings> fx_settings);

  // This is not the most optimal implementation: if an FX is added to
  // the list we consider we can't update it quickly (while we could
  // do a two stages init with new allocation outside the DSP path).
  //
  // It's simple enough for now though.
  bool CanFastUpdate(const std::list<Fx::Settings> fx_settings);
  void FastUpdate(const std::list<Fx::Settings> fx_settings);
  void Render(SampleTick tick, AudioBuffer& buffer);

  absl::Status OpenVstEditor(const std::string& fx_name);
  absl::Status CloseVstEditor(const std::string& fx_name);

 private:
  absl::StatusOr<FxVst*> FindVstFx(const std::string& fx_name);

  Controls* controls_;
  vst::VstHost* vst_host_;

  std::mutex mutex_;
  std::list<std::string> order_;
  std::map<std::string, std::unique_ptr<Fx>> fxs_;
};

}  // namespace fx
}  // namespace soir
