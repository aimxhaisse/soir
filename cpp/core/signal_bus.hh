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

// Inter-track audio signal bus.
//
// Every track publishes its rendered output (post-FX, pre-fader,
// pre-mute) here, block by block, and the engine publishes the mixed
// master signal. Any FX can read the most recent fully rendered block
// of any signal via Latest(), which is deterministically one block
// behind the one currently being rendered.
//
// The registry is append-only: Declare() interns a name forever and
// entries are never removed, so the Signal* handles cached by
// publishers and readers (tracks at Init, the engine for the master,
// the compressor in ReloadParams) stay valid for the lifetime of the
// bus — a reader of a removed track can never dangle. A signal that is
// not (anymore) published simply fails Latest()'s tick check and is
// reported as "no block", which is indistinguishable from the source
// not being present.
//
// Threading: there are two levels of mutexes, and no call ever holds
// one while taking the other, so deadlock is not possible:
//
// - The registry mutex (mutex_) protects the name -> Signal map. It is
//   only taken from setup threads (SetupTracks, FX parameter reloads),
//   never once per block.
//
// - Each signal has its own mutex protecting its ring. Publish/Latest
//   take only this lock, for the duration of one kBlockSize-sample
//   copy in or out (a few microseconds). A signal is published by
//   exactly one thread (its track, or the engine for the master) and
//   read by its sidechain consumers, so different signals never
//   contend with each other: a slow consumer of one track can never
//   delay another track's publish.
//
// Latest() copies the block out into caller-provided buffers, so no
// pointer into the ring ever escapes the lock.
//
// kDepth must stay >= 2: the reader of block N reads the slot of block
// N-1, which must not be the slot the publisher of block N writes.
class SignalBus {
 private:
  static constexpr SampleTick kNeverWritten =
      std::numeric_limits<SampleTick>::max();

  struct Slot {
    // kNeverWritten until the slot receives its first block, so a
    // fresh ring is never mistaken for a real (zero-filled) block 0.
    SampleTick tick = kNeverWritten;
    std::vector<float> left;
    std::vector<float> right;
  };

 public:
  static constexpr int kDepth = 4;

  // Per-signal ring of the last kDepth published blocks. Pointers to
  // it are stable for the lifetime of the bus; only ever used through
  // the methods below.
  struct Signal {
    // Protects the ring.
    mutable std::mutex mutex_;
    std::array<Slot, kDepth> slots;
  };

  // Setup threads only (take the registry lock).

  // Intern a signal name, pre-allocating its ring on first use.
  // Idempotent: the same name always returns the same stable handle,
  // so a track re-created with the same name resumes on the same
  // storage.
  Signal* Declare(const std::string& name);

  // Look up the handle of a signal. Returns nullptr only if the name
  // was never declared; a name whose track no longer exists still
  // returns its handle (its Latest() reads report "no block" until the
  // track is created again).
  Signal* Find(const std::string& name) const;

  // Audio threads (take only the signal's own mutex).

  // Publish the block starting at `tick`. Must be called once per
  // block per signal, in increasing tick order, from a single thread.
  // `size` must be within [1, kBlockSize]; null handles or oversized
  // blocks are dropped.
  static void Publish(Signal* signal, SampleTick tick, const float* left,
                      const float* right, int size);

  // Copy the block at `tick - kBlockSize`, i.e. the most recent fully
  // rendered block, into left and right (each must hold kBlockSize
  // samples). Returns false if `signal` is null, `tick` has no
  // previous block, or the signal never published a block at that
  // exact position (source track not present yet, just created, or
  // removed).
  static bool Latest(const Signal* signal, SampleTick tick, float* left,
                     float* right);

 private:
  // Protects the name registry.
  mutable std::mutex mutex_;
  std::map<std::string, std::unique_ptr<Signal>> signals_;
};

}  // namespace soir
