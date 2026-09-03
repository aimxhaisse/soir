#include "core/signal_bus.hh"

#include <algorithm>

namespace soir {

void SignalBus::Declare(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (signals_.find(name) != signals_.end()) {
    return;
  }

  auto signal = std::make_unique<Signal>();
  for (auto& slot : signal->slots) {
    slot.left.resize(kBlockSize);
    slot.right.resize(kBlockSize);
  }

  signals_.emplace(name, std::move(signal));
}

bool SignalBus::Declared(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return signals_.find(name) != signals_.end();
}

void SignalBus::Publish(const std::string& name, SampleTick tick,
                        const float* left, const float* right, int size) {
  if (left == nullptr || right == nullptr || size <= 0 || size > kBlockSize) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  if (it == signals_.end()) {
    return;
  }

  Slot& slot = it->second->slots[(tick / kBlockSize) % kDepth];
  std::copy_n(left, size, slot.left.data());
  std::copy_n(right, size, slot.right.data());
  slot.tick = tick;
}

bool SignalBus::Latest(const std::string& name, SampleTick tick, float* left,
                       float* right) {
  if (left == nullptr || right == nullptr || tick < kBlockSize) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  if (it == signals_.end()) {
    return false;
  }

  const Slot& slot =
      it->second->slots[(tick - kBlockSize) / kBlockSize % kDepth];
  if (slot.tick != tick - kBlockSize) {
    // Stale slot: no block was published at tick - kBlockSize.
    return false;
  }

  std::copy_n(slot.left.data(), kBlockSize, left);
  std::copy_n(slot.right.data(), kBlockSize, right);

  return true;
}

}  // namespace soir
