#pragma once

#include <absl/status/status.h>

#include <array>
#include <cstddef>
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

// A block of stereo samples published on the SignalBus.
struct Block {
  SampleTick tick = 0;  // Tick of the first sample in this block.
  const float* left = nullptr;
  const float* right = nullptr;
};

// Inter-track audio signal bus.
//
// Every track publishes its rendered output (post-FX, pre-fader,
// pre-mute) here, block by block, and the engine publishes the mixed
// master signal. Any FX can read the most recent fully rendered block
// of any signal via Latest(), which is deterministically one block
// behind the one currently being rendered.
//
// Threading: each signal is written by exactly one thread (its track
// thread, or the engine thread for the master), once per block, in
// increasing tick order. Readers only ever read the previous block,
// and the engine loop's join/dispatch hand-off orders "all of block N
// published" before "any of block N+1 rendered", so the sample data is
// read without locks. The mutex only protects the name registry.
//
// kDepth must stay above 2: it guarantees a writer never overwrites a
// slot a reader may still be consuming (a reader of block N reads the
// slot of block N-1, which is next written at block N+3).
class SignalBus {
 public:
  static constexpr int kDepth = 4;

  // Pre-allocate the ring for a signal name. Idempotent; re-declaring
  // a dying signal revives it (same storage, new publisher).
  void Declare(const std::string& name);

  // Mark a signal as removed: Latest() returns nullptr for it and
  // Publish() drops its blocks. The storage is kept for a few blocks
  // so readers still consuming the last published block can't dangle,
  // then it is reclaimed lazily.
  void Undeclare(const std::string& name);

  // Whether the signal is declared and alive.
  bool Declared(const std::string& name) const;

  // Publish the block starting at `tick`. Must be called once per
  // block per signal, in increasing tick order, from a single thread.
  void Publish(const std::string& name, SampleTick tick, const float* left,
               const float* right, int size);

  // Read the block at `tick - kBlockSize`, i.e. the most recent fully
  // rendered block. Returns nullptr if the signal is unknown, dying,
  // or never published a block at that exact position (source track
  // not present yet / just created). The pointer is valid until the
  // next Publish/Latest call of *this* signal more than kDepth blocks
  // later, which the engine loop ordering guarantees never happens
  // while a block is being rendered.
  const Block* Latest(const std::string& name, SampleTick tick) const;

 private:
  static constexpr SampleTick kNotDying =
      std::numeric_limits<SampleTick>::max();

  struct Slot {
    // kNeverWritten until the slot receives its first block, so a
    // fresh ring is never mistaken for a real (zero-filled) block 0.
    static constexpr SampleTick kNeverWritten =
        std::numeric_limits<SampleTick>::max();
    SampleTick tick = kNeverWritten;
    std::vector<float> left;
    std::vector<float> right;
    Block block;  // Points into left/right.
  };

  struct Signal {
    // Tick at which the signal was undeclared, kNotDying if alive.
    SampleTick dying_since = kNotDying;
    std::array<Slot, kDepth> slots;
  };

  // Free signals that have been dying for more than kDepth blocks.
  void ReapDying();

  // Protects the registry (and the dying state), never the sample
  // data itself.
  mutable std::mutex mutex_;
  std::map<std::string, std::unique_ptr<Signal>> signals_;

  // Highest tick ever published, used as the clock for lazy
  // reclamation of dying signals.
  SampleTick latest_tick_ = 0;
};

}  // namespace soir
