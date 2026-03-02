#include "fx/fx_vst.hh"

#include <absl/log/log.h>

#include <nlohmann/json.hpp>

#include "core/common.hh"

namespace soir {
namespace vst {

extern void* CreateEditorWindow(int width, int height, const char* title);
extern void ResizeEditorWindow(void* view, int width, int height);
extern void ShowEditorWindow(void* view);
extern void CloseEditorWindow(void* view);

}  // namespace vst

namespace fx {

FxVst::FxVst(Controls* controls, vst::VstHost* vst_host)
    : controls_(controls), vst_host_(vst_host), initialized_(false) {}

absl::Status FxVst::Init(const Fx::Settings& settings) {
  settings_ = settings;

  auto doc = nlohmann::json::parse(settings_.extra_, nullptr, false);
  if (doc.is_discarded()) {
    return absl::InvalidArgumentError("Failed to parse JSON: " +
                                      settings_.extra_);
  }

  if (!doc.contains("plugin")) {
    return absl::InvalidArgumentError("VST effect missing 'plugin' field");
  }

  plugin_name_ = doc["plugin"].get<std::string>();

  if (!vst_host_) {
    return absl::FailedPreconditionError("VST host not available");
  }

  auto result = vst_host_->LoadPlugin(plugin_name_);
  if (!result.ok()) {
    return result.status();
  }

  plugin_ = std::move(*result);

  auto status = plugin_->Activate(kSampleRate, kBlockSize);
  if (!status.ok()) {
    return status;
  }

  ReloadParams();
  initialized_ = true;

  LOG(INFO) << "Initialized VST effect: " << plugin_name_;
  return absl::OkStatus();
}

bool FxVst::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  auto doc = nlohmann::json::parse(settings.extra_, nullptr, false);
  if (doc.is_discarded() || !doc.contains("plugin")) {
    return false;
  }

  std::string new_name = doc["plugin"].get<std::string>();
  return new_name == plugin_name_;
}

void FxVst::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void FxVst::ReloadParams() {
  auto doc = nlohmann::json::parse(settings_.extra_, nullptr, false);
  if (doc.is_discarded()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  automated_params_.clear();

  if (!doc.contains("params") || !plugin_) {
    return;
  }

  auto vst_params = plugin_->GetParameters();

  for (auto& [param_name, ref] : doc["params"].items()) {
    auto vst_it = vst_params.find(param_name);
    if (vst_it != vst_params.end()) {
      AutomatedParam ap;
      ap.vst_param_id = vst_it->second.id;

      if (ref.is_string()) {
        ap.param.SetControl(controls_, ref.get<std::string>());
      } else if (ref.is_number()) {
        ap.param.SetConstant(static_cast<float>(ref.get<double>()));
      }
      ap.param.SetRange(0.0f, 1.0f);

      automated_params_[param_name] = ap;
    }
  }
}

void FxVst::Render(SampleTick tick, AudioBuffer& buffer,
                   const std::list<MidiEventAt>& events) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || !plugin_) {
    return;
  }

  for (auto& [name, ap] : automated_params_) {
    float value = ap.param.GetValue(tick);
    plugin_->SetParameter(ap.vst_param_id, value);
  }

  plugin_->Process(tick, buffer, events);
}

absl::Status FxVst::OpenEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!plugin_) {
    return absl::FailedPreconditionError("Plugin not loaded");
  }

  if (editor_window_) {
    plugin_->CloseEditor();
    editor_window_.reset();
  }

  auto* view = vst::CreateEditorWindow(800, 600, plugin_name_.c_str());
  if (!view) {
    return absl::InternalError("Failed to create editor window");
  }

  auto status = plugin_->OpenEditor(view);
  if (!status.ok()) {
    vst::CloseEditorWindow(view);
    return status;
  }

  auto [w, h] = plugin_->GetEditorSize();
  vst::ResizeEditorWindow(view, w, h);

  editor_window_ = std::unique_ptr<void, std::function<void(void*)>>(
      view, [](void* v) { vst::CloseEditorWindow(v); });

  vst::ShowEditorWindow(view);
  return absl::OkStatus();
}

absl::Status FxVst::CloseEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!plugin_) {
    return absl::OkStatus();
  }

  auto status = plugin_->CloseEditor();
  editor_window_.reset();
  return status;
}

bool FxVst::IsEditorOpen() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!plugin_) {
    return false;
  }

  return plugin_->IsEditorOpen();
}

}  // namespace fx
}  // namespace soir
