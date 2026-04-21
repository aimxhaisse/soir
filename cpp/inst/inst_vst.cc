#include "inst/inst_vst.hh"

#include <absl/log/log.h>

#include <nlohmann/json.hpp>

#include "core/common.hh"

namespace soir {
namespace inst {

InstVst::InstVst(vst::VstHost* vst_host) : vst_host_(vst_host) {}

absl::Status InstVst::Init(const std::string& settings,
                           SampleManager* /*sample_manager*/,
                           Controls* controls) {
  controls_ = controls;
  settings_json_ = settings;

  auto doc = nlohmann::json::parse(settings, nullptr, false);
  if (doc.is_discarded()) {
    return absl::InvalidArgumentError("Failed to parse JSON: " + settings);
  }

  if (!doc.contains("plugin")) {
    return absl::InvalidArgumentError("VST instrument missing 'plugin' field");
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

  LOG(INFO) << "Initialized VST instrument: " << plugin_name_;
  return absl::OkStatus();
}

absl::Status InstVst::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (plugin_) {
    plugin_->CloseEditor().IgnoreError();
    plugin_->Deactivate().IgnoreError();
    plugin_->Shutdown().IgnoreError();
    plugin_.reset();
  }
  initialized_ = false;
  return absl::OkStatus();
}

void InstVst::ReloadParams() {
  auto doc = nlohmann::json::parse(settings_json_, nullptr, false);
  if (doc.is_discarded()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_json_;
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

void InstVst::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                     AudioBuffer& buffer) {
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

}  // namespace inst
}  // namespace soir
