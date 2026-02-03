#include "fx/fx_vst.hh"

#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/common.hh"

namespace soir {
namespace fx {

FxVst::FxVst(Controls* controls, vst::VstHost* vst_host)
    : controls_(controls), vst_host_(vst_host), initialized_(false) {}

absl::Status FxVst::Init(const Fx::Settings& settings) {
  settings_ = settings;

  rapidjson::Document doc;
  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    return absl::InvalidArgumentError("Failed to parse JSON: " +
                                      settings_.extra_);
  }

  if (!doc.HasMember("plugin")) {
    return absl::InvalidArgumentError("VST effect missing 'plugin' field");
  }

  plugin_uid_ = doc["plugin"].GetString();

  if (!vst_host_) {
    return absl::FailedPreconditionError("VST host not available");
  }

  auto result = vst_host_->LoadPlugin(plugin_uid_);
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

  LOG(INFO) << "Initialized VST effect: " << plugin_uid_;
  return absl::OkStatus();
}

bool FxVst::CanFastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.type_ != settings.type_) {
    return false;
  }

  rapidjson::Document doc;
  doc.Parse(settings.extra_.c_str());
  if (doc.HasParseError() || !doc.HasMember("plugin")) {
    return false;
  }

  std::string new_uid = doc["plugin"].GetString();
  return new_uid == plugin_uid_;
}

void FxVst::FastUpdate(const Fx::Settings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings_.extra_ != settings.extra_) {
    settings_ = settings;
    ReloadParams();
  }
}

void FxVst::ReloadParams() {
  rapidjson::Document doc;
  doc.Parse(settings_.extra_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_.extra_;
    return;
  }

  automated_params_.clear();

  if (!doc.HasMember("params") || !plugin_) {
    return;
  }

  auto vst_params = plugin_->GetParameters();
  const auto& params = doc["params"];

  for (auto it = params.MemberBegin(); it != params.MemberEnd(); ++it) {
    std::string param_name = it->name.GetString();

    auto vst_it = vst_params.find(param_name);
    if (vst_it != vst_params.end()) {
      AutomatedParam ap;
      ap.vst_param_id = vst_it->second.id;

      // Build Parameter from the Value directly
      const rapidjson::Value& ref = it->value;
      if (ref.IsString()) {
        ap.param.SetControl(controls_, ref.GetString());
      } else if (ref.IsNumber()) {
        ap.param.SetConstant(static_cast<float>(ref.GetDouble()));
      }
      ap.param.SetRange(0.0f, 1.0f);

      automated_params_[param_name] = ap;
    }
  }
}

void FxVst::Render(SampleTick tick, AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || !plugin_) {
    return;
  }

  for (auto& [name, ap] : automated_params_) {
    float value = ap.param.GetValue(tick);
    plugin_->SetParameter(ap.vst_param_id, value);
  }

  plugin_->Process(buffer);
}

absl::Status FxVst::OpenEditor(void* parent_window) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!plugin_) {
    return absl::FailedPreconditionError("Plugin not loaded");
  }

  return plugin_->OpenEditor(parent_window);
}

absl::Status FxVst::CloseEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!plugin_) {
    return absl::OkStatus();
  }

  return plugin_->CloseEditor();
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
