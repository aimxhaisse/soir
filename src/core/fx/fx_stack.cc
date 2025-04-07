#include <absl/log/log.h>

#include "fx_chorus.hh"
#include "fx_reverb.hh"

#include "fx_stack.hh"

namespace soir {
namespace fx {

FxStack::FxStack(Controls* controls) : controls_(controls) {}

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

void FxStack::Render(SampleTick tick, AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& name : order_) {
    auto fx = fxs_.find(name);
    if (fx == fxs_.end()) {
      continue;
    }

    {
      const std::string trace_name = "fx::" + name;
      SOIR_TRACING_ZONE_COLOR_STR(trace_name, SOIR_ORANGE);
      fx->second->Render(tick, buffer);
    }

    SOIR_TRACING_FRAME("fx::stack");
  }
}

}  // namespace fx
}  // namespace soir
