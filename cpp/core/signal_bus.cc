#include "core/signal_bus.hh"

#include <algorithm>

namespace soir {

void SignalBus::Declare(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  if (it != signals_.end()) {
    // Revive a dying signal in place: its storage must outlive readers
    // of its last published block, so it is reused rather than
    // replaced. Stale slots are not returned by Latest() because they
    // no longer match the expected tick.
    it->second->dying_since = kNotDying;
    return;
  }

  auto signal = std::make_unique<Signal>();
  for (auto& slot : signal->slots) {
    slot.left.resize(kBlockSize);
    slot.right.resize(kBlockSize);
  }

  signals_[name] = std::move(signal);
  ReapDying();
}

void SignalBus::Undeclare(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  if (it == signals_.end()) {
    return;
  }

  if (it->second->dying_since == kNotDying) {
    it->second->dying_since = latest_tick_;
  }

  ReapDying();
}

bool SignalBus::Declared(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = signals_.find(name);
  return it != signals_.end() && it->second->dying_since == kNotDying;
}

void SignalBus::Publish(const std::string& name, SampleTick tick,
                        const float* left, const float* right, int size) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (tick > latest_tick_) {
    latest_tick_ = tick;
  }
  ReapDying();

  auto it = signals_.find(name);
  if (it == signals_.end() || it->second->dying_since != kNotDying) {
    // The signal was never declared (or is dying): drop the block.
    return;
  }

  Signal& signal = *it->second;
  Slot& slot = signal.slots[(tick / kBlockSize) % kDepth];

  if (slot.left.size() != static_cast<size_t>(size)) {
    slot.left.resize(static_cast<size_t>(size));
    slot.right.resize(static_cast<size_t>(size));
  }

  std::copy(left, left + size, slot.left.begin());
  std::copy(right, right + size, slot.right.begin());

  slot.tick = tick;
  slot.block.tick = tick;
  slot.block.left = slot.left.data();
  slot.block.right = slot.right.data();
}

const Block* SignalBus::Latest(const std::string& name, SampleTick tick) const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (tick < kBlockSize) {
    return nullptr;
  }

  auto it = signals_.find(name);
  if (it == signals_.end() || it->second->dying_since != kNotDying) {
    return nullptr;
  }

  const Slot& slot =
      it->second->slots[(tick - kBlockSize) / kBlockSize % kDepth];
  if (slot.tick != tick - kBlockSize) {
    // The source never published a block at that position (not present
    // yet, or just created): treat it as missing.
    return nullptr;
  }

  return &slot.block;
}

void SignalBus::ReapDying() {
  for (auto it = signals_.begin(); it != signals_.end();) {
    if (it->second->dying_since != kNotDying &&
        latest_tick_ >= it->second->dying_since + kDepth * kBlockSize) {
      it = signals_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace soir
