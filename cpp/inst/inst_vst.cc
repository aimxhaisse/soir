#include "inst/inst_vst.hh"

#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/common.hh"

namespace soir {
namespace inst {

InstVst::InstVst(vst::VstHost* vst_host) : vst_host_(vst_host) {}

absl::Status InstVst::Init(const std::string& settings,
                           SampleManager* /*sample_manager*/,
                           Controls* controls) {
  controls_ = controls;
  settings_json_ = settings;

  rapidjson::Document doc;
  doc.Parse(settings.c_str());
  if (doc.HasParseError()) {
    return absl::InvalidArgumentError("Failed to parse JSON: " + settings);
  }

  if (!doc.HasMember("plugin")) {
    return absl::InvalidArgumentError("VST instrument missing 'plugin' field");
  }

  plugin_name_ = doc["plugin"].GetString();

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

void InstVst::ReloadParams() {
  rapidjson::Document doc;
  doc.Parse(settings_json_.c_str());
  if (doc.HasParseError()) {
    LOG(ERROR) << "Failed to parse JSON: " << settings_json_;
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

  plugin_->Process(buffer, events);
}

}  // namespace inst
}  // namespace soir
