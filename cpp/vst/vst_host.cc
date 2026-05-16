#include "vst/vst_host.hh"

#include <absl/log/log.h>

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "vst/vst_editor.hh"
#include "vst/vst_plugin.hh"
#include "vst/vst_scanner.hh"

namespace soir {
namespace vst {

using namespace Steinberg;

// HostContext implementation

HostContext::HostContext() = default;

HostContext::~HostContext() = default;

tresult PLUGIN_API HostContext::getName(Vst::String128 name) {
  const char16_t soir_name[] = u"Soir";
  std::copy(std::begin(soir_name), std::end(soir_name), name);
  return kResultOk;
}

tresult PLUGIN_API HostContext::createInstance(TUID cid, TUID iid, void** obj) {
  // The VST3 specification allows plug-ins to ask the host to instantiate
  // IMessage and IAttributeList for inter-component communication. Several
  // plug-in frameworks (DPF, JUCE) rely on this. We delegate to the SDK's
  // reference implementations.
  if (FUnknownPrivate::iidEqual(cid, Vst::IMessage::iid) &&
      FUnknownPrivate::iidEqual(iid, Vst::IMessage::iid)) {
    *obj = new Vst::HostMessage();
    return kResultTrue;
  }
  if (FUnknownPrivate::iidEqual(cid, Vst::IAttributeList::iid) &&
      FUnknownPrivate::iidEqual(iid, Vst::IAttributeList::iid)) {
    if (auto al = Vst::HostAttributeList::make()) {
      *obj = al.take();
      return kResultTrue;
    }
    return kOutOfMemory;
  }
  *obj = nullptr;
  return kNotImplemented;
}

tresult PLUGIN_API HostContext::queryInterface(const TUID iid, void** obj) {
  if (FUnknownPrivate::iidEqual(iid, Vst::IHostApplication::iid) ||
      FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
    *obj = static_cast<Vst::IHostApplication*>(this);
    addRef();
    return kResultOk;
  }
  *obj = nullptr;
  return kNoInterface;
}

uint32 PLUGIN_API HostContext::addRef() { return ++ref_count_; }

uint32 PLUGIN_API HostContext::release() {
  auto count = --ref_count_;
  if (count == 0) {
    delete this;
  }
  return count;
}

// VstHost implementation

VstHost::VstHost() : initialized_(false) {}

VstHost::~VstHost() { Shutdown(); }

absl::Status VstHost::Init() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (initialized_) {
    return absl::OkStatus();
  }

  // Plugin scanning instantiates each plugin's IComponent, which on Linux
  // can reach into XLib (e.g. Cardinal's MIT-SHM cleanup). Install our error
  // handler before that happens so transient X errors do not kill the host.
  EditorWindow::InitPlatform();

  host_context_ = std::make_unique<HostContext>();
  initialized_ = true;
  LOG(INFO) << "VST host initialized";

  return absl::OkStatus();
}

absl::Status VstHost::Shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_) {
    return absl::OkStatus();
  }

  plugins_.clear();
  host_context_.reset();
  initialized_ = false;
  LOG(INFO) << "VST host shutdown";

  return absl::OkStatus();
}

FUnknown* VstHost::GetHostContext() { return host_context_.get(); }

absl::Status VstHost::ScanPlugins() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_) {
    return absl::FailedPreconditionError("VST host not initialized");
  }

  VstScanner scanner;
  std::map<std::string, PluginInfo> plugins;

  auto status = scanner.Scan(&plugins);
  if (!status.ok()) {
    return status;
  }

  plugins_ = std::move(plugins);
  LOG(INFO) << "Found " << plugins_.size() << " VST plugins";

  return absl::OkStatus();
}

std::map<std::string, PluginInfo> VstHost::GetAvailablePlugins() {
  std::lock_guard<std::mutex> lock(mutex_);
  return plugins_;
}

absl::StatusOr<PluginInfo> VstHost::GetPlugin(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = plugins_.find(name);
  if (it != plugins_.end()) {
    return it->second;
  }

  return absl::NotFoundError("Plugin not found: " + name);
}

absl::StatusOr<std::unique_ptr<VstPlugin>> VstHost::LoadPlugin(
    const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = plugins_.find(name);
  if (it == plugins_.end()) {
    return absl::NotFoundError("Plugin not found: " + name);
  }

  auto plugin = std::make_unique<VstPlugin>();
  auto status = plugin->Init(it->second.path, it->second.uid,
                             host_context_.get(), it->second.type);
  if (!status.ok()) {
    return status;
  }

  return plugin;
}

}  // namespace vst
}  // namespace soir
