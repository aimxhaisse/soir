#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "core/parameter.hh"
#include "inst/instrument.hh"
#include "vst/vst_host.hh"
#include "vst/vst_plugin.hh"

namespace soir {
namespace inst {

class InstVst : public Instrument {
 public:
  InstVst(vst::VstHost* vst_host);

  absl::Status Init(const std::string& settings, SampleManager* sample_manager,
                    Controls* controls) override;
  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer) override;
  Type GetType() const override { return Type::VST; }
  std::string GetName() const override { return "VST:" + plugin_name_; }

  vst::VstPlugin* GetPlugin() { return plugin_.get(); }

 private:
  void ReloadParams();

  vst::VstHost* vst_host_;
  Controls* controls_ = nullptr;

  std::mutex mutex_;
  std::string settings_json_;
  std::string plugin_name_;
  std::unique_ptr<vst::VstPlugin> plugin_;

  struct AutomatedParam {
    Parameter param;
    uint32_t vst_param_id;
  };
  std::map<std::string, AutomatedParam> automated_params_;

  bool initialized_ = false;
};

}  // namespace inst
}  // namespace soir
