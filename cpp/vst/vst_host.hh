#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivsthostapplication.h"

namespace soir {
namespace vst {

enum class VstPluginType { kVstFx, kVstInstrument };

struct PluginInfo {
  std::string uid;
  std::string name;
  std::string vendor;
  std::string category;
  std::string path;
  int num_audio_inputs;
  int num_audio_outputs;
  VstPluginType type = VstPluginType::kVstFx;
};

class VstPlugin;

// Base host application context for VST3 plugins.
// Platform-specific extensions (e.g. Linux IRunLoop) are provided by derived
// classes in the per-platform translation units.
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

 protected:
  // Hook for platform-specific interfaces (e.g. Linux::IRunLoop).
  virtual Steinberg::tresult QueryPlatformInterface(const Steinberg::TUID iid,
                                                    void** obj);

 private:
  std::atomic<Steinberg::int32> ref_count_{1};
};

// Factory — returns the appropriate concrete type for the platform.
HostContext* CreateHostContext();

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
  HostContext* host_context_ = nullptr;
};

}  // namespace vst
}  // namespace soir
