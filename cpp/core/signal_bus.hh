#pragma once

#include <array>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "core/common.hh"

namespace soir {

// Name of the internal signal carrying the master mix, published by
// the engine. A compressor configured with source="master" reads it.
static constexpr std::string_view kMasterSignal = "soir_internal_master";

// Inter-track audio signal bus.
//
// Every track publishes its rendered output (post-FX, pre-fader,
// pre-mute) block by block; the engine publishes the master mix under
// kMasterSignal. Latest(name, tick) copies out the block at
// tick - kBlockSize: the one-block delay comes from the engine
// dispatch/join ordering, not from this class.
//
// One mutex guards the registry and the rings, held for one kBlockSize
// copy in or out; the copy-out keeps DSP off the lock. The registry is
// append-only (16 KB per name), so removed names simply stop being
// served (stale slots fail the tick check). kDepth must stay >= 2:
// block N reads the slot of N-1, which must not be the slot N writes.
class SignalBus {
 public:
  static constexpr int kDepth = 4;

  // Intern a name, pre-allocating its ring. Idempotent.
  void Declare(const std::string& name);

  // Whether the name was ever declared. Append-only: stays true after
  // the track is removed, so this distinguishes a typo from a source
  // that exists but has not published a block at the expected tick.
  bool Declared(const std::string& name) const;

  // Publish the block starting at `tick`. Once per block, increasing
  // order, single thread. Unknown names and sizes outside
  // [1, kBlockSize] are dropped.
  void Publish(const std::string& name, SampleTick tick, const float* left,
               const float* right, int size);

  // Copy the block at `tick - kBlockSize` into left/right (each must
  // hold kBlockSize samples). Returns false if the name is unknown or
  // has no block at that position.
  bool Latest(const std::string& name, SampleTick tick, float* left,
              float* right);

 private:
  static constexpr SampleTick kNeverWritten =
      std::numeric_limits<SampleTick>::max();

  struct Slot {
    // kNeverWritten until first publish: a fresh ring is not a real
    // (zero-filled) block 0.
    SampleTick tick = kNeverWritten;
    std::vector<float> left;
    std::vector<float> right;
  };

  struct Signal {
    std::array<Slot, kDepth> slots;
  };

  // Protects the registry and the rings. Mutable: Declared() is const.
  mutable std::mutex mutex_;
  std::map<std::string, std::unique_ptr<Signal>> signals_;
};

}  // namespace soir
