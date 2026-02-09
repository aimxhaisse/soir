#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivsthostapplication.h"

namespace soir {
namespace vst {

struct PluginInfo {
  std::string uid;
  std::string name;
  std::string vendor;
  std::string category;
  std::string path;
  int num_audio_inputs;
  int num_audio_outputs;
};

class VstPlugin;

// Simple host application context for VST3 plugins.
class HostContext : public Steinberg::Vst::IHostApplication {
 public:
  HostContext();
  virtual ~HostContext();

  // IHostApplication
  Steinberg::tresult PLUGIN_API
  getName(Steinberg::Vst::String128 name) override;
  Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid,
                                               Steinberg::TUID iid,
                                               void** obj) override;

  // FUnknown
  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,
                                               void** obj) override;
  Steinberg::uint32 PLUGIN_API addRef() override;
  Steinberg::uint32 PLUGIN_API release() override;

 private:
  std::atomic<Steinberg::int32> ref_count_{1};
};

class VstHost {
 public:
  VstHost();
  ~VstHost();

  absl::Status Init();
  absl::Status Shutdown();

  absl::Status ScanPlugins();
  std::map<std::string, PluginInfo> GetAvailablePlugins();
  absl::StatusOr<PluginInfo> GetPlugin(const std::string& name);

  absl::StatusOr<std::unique_ptr<VstPlugin>> LoadPlugin(
      const std::string& name);

  // Get the host context for plugin initialization.
  Steinberg::FUnknown* GetHostContext();

 private:
  std::mutex mutex_;
  bool initialized_ = false;
  std::map<std::string, PluginInfo> plugins_;
  std::unique_ptr<HostContext> host_context_;
};

}  // namespace vst
}  // namespace soir
