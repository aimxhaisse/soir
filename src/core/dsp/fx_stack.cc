#include <absl/log/log.h>

#include "fx_stack.hh"

namespace soir {
namespace dsp {

FxStack::FxStack() {}

absl::Status FxStack::Init(const std::list<Fx::Settings> fx_settings) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& settings : fx_settings) {
    auto fx = std::make_unique<Fx>();
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

absl::Status FxStack::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& it : fxs_) {
    auto status = it.second->Stop();
    if (!status.ok()) {
      LOG(WARNING) << "Unable to stop FX '" << it.first
                   << "' properly, skipped";
    }
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
    fx->second->Render(tick, buffer);
  }
}

}  // namespace dsp
}  // namespace soir
