#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "core/parameter.hh"
#include "fx/fx.hh"
#include "vst/vst_host.hh"
#include "vst/vst_plugin.hh"

namespace soir {
namespace fx {

struct FxVst : public Fx {
  FxVst(Controls* controls, vst::VstHost* vst_host);

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer& buffer) override;

  absl::Status OpenEditor();
  absl::Status CloseEditor();
  bool IsEditorOpen();

 private:
  void ReloadParams();

  Controls* controls_;
  vst::VstHost* vst_host_;

  std::mutex mutex_;
  Fx::Settings settings_;

  std::unique_ptr<vst::VstPlugin> plugin_;
  std::string plugin_name_;

  struct AutomatedParam {
    Parameter param;
    uint32_t vst_param_id;
  };
  std::map<std::string, AutomatedParam> automated_params_;

  bool initialized_ = false;

  std::unique_ptr<void, std::function<void(void*)>> editor_window_;
};

}  // namespace fx
}  // namespace soir
