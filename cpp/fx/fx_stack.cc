#include "fx_stack.hh"

#include <absl/log/log.h>

#include "fx_chorus.hh"
#include "fx_hpf.hh"
#include "fx_lpf.hh"
#include "fx_reverb.hh"
#include "fx_vst.hh"

namespace soir {
namespace fx {

FxStack::FxStack(Controls* controls, vst::VstHost* vst_host)
    : controls_(controls), vst_host_(vst_host) {}

absl::Status FxStack::Init(const std::list<Fx::Settings> fx_settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::unique_ptr<Fx> fx;

  for (auto& settings : fx_settings) {
    switch (settings.type_) {
      case Type::CHORUS:
        fx = std::make_unique<Chorus>(controls_);
        break;
      case Type::REVERB:
        fx = std::make_unique<Reverb>(controls_);
        break;
      case Type::LPF:
        fx = std::make_unique<LPF>(controls_);
        break;
      case Type::HPF:
        fx = std::make_unique<HPF>(controls_);
        break;
      case Type::VST:
        fx = std::make_unique<FxVst>(controls_, vst_host_);
        break;
      default:
        return absl::InvalidArgumentError("Unknown FX type");
    }

    auto status = fx->Init(settings);

    if (!status.ok()) {
      order_.clear();
      fxs_.clear();
      return status;
    }

    fxs_[settings.name_] = std::move(fx);
    order_.push_back(settings.name_);

    LOG(INFO) << "Initialized FX '" << settings.name_ << "'";
  }

  return absl::OkStatus();
}

bool FxStack::CanFastUpdate(const std::list<Fx::Settings> fx_settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& settings : fx_settings) {
    const auto& name = settings.name_;
    const auto& it = fxs_.find(name);

    if (it == fxs_.end() || !it->second->CanFastUpdate(settings)) {
      return false;
    }
  }

  return true;
}

void FxStack::FastUpdate(const std::list<Fx::Settings> fx_settings) {
  std::list<std::string> order;
  std::map<std::string, std::unique_ptr<Fx>> fxs;
  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& settings : fx_settings) {
    auto it = fxs_.find(settings.name_);
    if (it != fxs_.end()) {
      auto& fx = it->second;
      fx->FastUpdate(settings);
      order.push_back(settings.name_);
      fxs[settings.name_] = std::move(fx);
    }
  }

  fxs_.swap(fxs);
  order_.swap(order);
}

void FxStack::Render(SampleTick tick, AudioBuffer& buffer,
                     const std::list<MidiEventAt>& events) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& name : order_) {
    auto fx = fxs_.find(name);
    if (fx == fxs_.end()) {
      continue;
    }

    {
      const std::string trace_name = "fx::" + name;
      SOIR_TRACING_ZONE_COLOR_STR(trace_name, SOIR_ORANGE);
      fx->second->Render(tick, buffer, events);
    }

    SOIR_TRACING_FRAME("fx::stack");
  }
}

absl::StatusOr<FxVst*> FxStack::FindVstFx(const std::string& fx_name) {
  auto it = fxs_.find(fx_name);
  if (it == fxs_.end()) {
    return absl::NotFoundError("Effect not found: " + fx_name);
  }
  auto* vst_fx = dynamic_cast<FxVst*>(it->second.get());
  if (!vst_fx) {
    return absl::InvalidArgumentError("Effect is not a VST: " + fx_name);
  }
  return vst_fx;
}

absl::Status FxStack::OpenVstEditor(const std::string& fx_name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto result = FindVstFx(fx_name);
  if (!result.ok()) return result.status();
  return (*result)->OpenEditor();
}

absl::Status FxStack::CloseVstEditor(const std::string& fx_name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto result = FindVstFx(fx_name);
  if (!result.ok()) return result.status();
  return (*result)->CloseEditor();
}

}  // namespace fx
}  // namespace soir
