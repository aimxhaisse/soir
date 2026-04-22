#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "core/parameter.hh"
#include "fx/fx.hh"
#include "vst/vst_editor.hh"
#include "vst/vst_host.hh"
#include "vst/vst_plugin.hh"

namespace soir {
namespace fx {

struct FxVst : public Fx {
  FxVst(Controls* controls, vst::VstHost* vst_host);
  ~FxVst();

  absl::Status Init(const Fx::Settings& settings) override;
  bool CanFastUpdate(const Fx::Settings& settings) override;
  void FastUpdate(const Fx::Settings& settings) override;
  void Render(SampleTick tick, AudioBuffer& buffer,
              const std::list<MidiEventAt>& events) override;

  absl::Status OpenEditor();
  absl::Status CloseEditor();
  bool IsEditorOpen();

 private:
  void ReloadParams();

  Controls* controls_;
  vst::VstHost* vst_host_;

  std::mutex mutex_;
  Fx::Settings settings_;

  // editor_window_ must be declared before plugin_ so that the destructor
  // destroys plugin_ first (component_->terminate(), Wine exits) and then
  // editor_window_ (XDestroyWindow). Destroying the window while Wine is still
  // alive triggers a DestroyNotify that can cause a Wine stack overflow on the
  // second attach/detach cycle.
  std::unique_ptr<vst::EditorWindow> editor_window_;
  std::unique_ptr<vst::VstPlugin> plugin_;
  std::string plugin_name_;

  struct AutomatedParam {
    Parameter param;
    uint32_t vst_param_id;
  };
  std::map<std::string, AutomatedParam> automated_params_;

  bool initialized_ = false;
};

}  // namespace fx
}  // namespace soir
