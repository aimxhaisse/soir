#include "core/signal_bus.hh"

#include <algorithm>

namespace soir {

SignalBus::Signal* SignalBus::Declare(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  if (it == signals_.end()) {
    auto signal = std::make_unique<Signal>();
    for (auto& slot : signal->slots) {
      slot.left.resize(kBlockSize);
      slot.right.resize(kBlockSize);
    }
    it = signals_.emplace(name, std::move(signal)).first;
  }

  return it->second.get();
}

SignalBus::Signal* SignalBus::Find(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  return (it != signals_.end()) ? it->second.get() : nullptr;
}

void SignalBus::Publish(Signal* signal, SampleTick tick, const float* left,
                        const float* right, int size) {
  if (signal == nullptr || left == nullptr || right == nullptr || size <= 0 ||
      size > kBlockSize) {
    return;
  }

  std::lock_guard<std::mutex> lock(signal->mutex_);

  Slot& slot = signal->slots[(tick / kBlockSize) % kDepth];
  std::copy_n(left, size, slot.left.data());
  std::copy_n(right, size, slot.right.data());
  slot.tick = tick;
}

bool SignalBus::Latest(const Signal* signal, SampleTick tick, float* left,
                       float* right) {
  if (signal == nullptr || tick < kBlockSize) {
    return false;
  }

  std::lock_guard<std::mutex> lock(signal->mutex_);

  const Slot& slot = signal->slots[(tick - kBlockSize) / kBlockSize % kDepth];
  if (slot.tick != tick - kBlockSize) {
    // The source never published a block at that position (not present
    // yet, or just created): treat it as missing.
    return false;
  }

  std::copy_n(slot.left.data(), kBlockSize, left);
  std::copy_n(slot.right.data(), kBlockSize, right);

  return true;
}

}  // namespace soir
