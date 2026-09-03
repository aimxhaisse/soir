# Audio robustness: sidechain support and overrun handling

**Status:** as-built (2026-09-01), all changes committed in
`feat: working fix for the crackles, to be simplified`.

Companion to `designs/SIDECHAIN.md`: that document describes the
sidechain *feature* (a compressor keyed on another track, the master mix,
or its own input). This document describes what had to change in the
audio hot path to make that feature — and the engine in general —
reliable in the real world, and the overrun/underrun handling that
results.

## Background

Target environment: a VM, an external USB audio card, and an external
synth connected to it (MIDI out to the synth, the synth's audio returned
through the card and read back by an `External` track). Live coding drives
a muted "ghost" track that acts as a sidechain trigger for a compressor
FX on other tracks.

Symptom: persistent crackles in the audio heard on the card, even though
the affected signal (the synth's returned audio, re-emitted on the card's
output) was being processed correctly block by block.

The fix happened in three rounds:

1. **Round 1 — the sidechain feature itself was the crackle.** The
   original `SignalBus` took one global mutex on every publish/read from
   every audio thread, the engine's per-block dispatch took
   `Track::mutex_` via `GetTrackName()` (the same lock `FastUpdate` holds
   across a full instrument+FX re-init on every live-code edit), and a
   missing sidechain source logged from the audio thread.
2. **Round 2 — the capture path.** `External` configured its capture
   device without a period, so miniaudio/ALSA negotiated a 100 ms period
   (4800 frames) into a ring that held 4 blocks (2048 frames): roughly
   half of every external audio block was dropped, constantly. On top of
   that, the engine's soft clock ran on the wall clock (stepped by NTP /
   VM time sync), and underruns and input drops were completely
   invisible.
3. **Round 3 — simplification.** The bus's `dying`/`Undeclare`/`Declared`
   state machine (round 1) was removed: it duplicated what `Latest()`'s
   tick check already guarantees, and the append-only registry is strictly
   simpler.

## Root causes (why each thing below exists)

| # | Problem | Audible effect |
|---|---------|----------------|
| 1 | Global `SignalBus` mutex taken per block by every track, the engine, and every compressor | Contention + a `Declare` from the RT thread (any `tracks.setup()`) blocking all audio threads |
| 2 | Engine dispatch called `Track::GetTrackName()` per block → `Track::mutex_`, held by `FastUpdate` across full re-init on **every** live-code edit | Per-edit burst of underruns |
| 3 | `LOG(WARNING)` from the audio thread (missing source, once/s) | Click bursts (global log lock + stderr) |
| 4 | Capture period 100 ms vs 4-block ring (~50 % of input dropped) | Constant crackle in the external audio |
| 5 | Wall-clock (`system_clock`) soft clocks | VM time steps break the block schedule → underruns |
| 6 | Underruns/drops invisible; output ring unbounded | Could not diagnose; unbounded memory/latency after a stall |

## As-built design

### 1. Signal bus (`core/signal_bus.hh/.cc`) — append-only

A registry of named per-track signals (plus the internal `master`
signal), each holding a ring of the last `kDepth = 4` published blocks
(512 samples × 2 ch × 4 B = 16 KB per signal).

Invariants:

- **Append-only registry.** `Declare(name)` interns a name forever
  (find-or-create, one lock); entries are never removed. The only real
  requirement of the bus is that a cached `Signal*` never dangles, and
  "never free the storage" satisfies it — nothing else is needed.
- **`Latest()`'s tick check is the sole definition of "signal
  available".** A reader rendering block N reads the slot of block N-1
  and only accepts it if `slot.tick == tick - kBlockSize` (the tick is
  initialised to a sentinel so a fresh slot is never mistaken for a real
  zero-filled block 0). A signal whose track was removed, or has not
  started publishing yet, simply fails the check → the compressor passes
  through untouched.
- **Two mutex levels, never held together (no deadlock possible).**
  The registry mutex is taken only from setup threads (`Declare`/`Find`:
  `SetupTracks`, FX parameter reloads) — never per block. Each signal has
  its own mutex protecting its ring; `Publish`/`Latest` hold it for one
  512-sample copy in or out (a few µs). A signal is published by exactly
  one thread (its track, or the engine for `master`) and read by its
  sidechain consumers, so *different signals never contend*: a slow
  consumer of one track can never delay another track's publish.
- **`Latest()` copies the block out** into caller-provided buffers (the
  compressor holds 8 KB of scratch per instance), so no pointer into the
  ring ever escapes the lock.
- `kDepth ≥ 2` is required so the reader of block N never reads the slot
  the publisher of block N writes; 4 is used for slack.

What was deliberately *not* kept: the round-1 `dying` flag + `Undeclare`
+ `Declared`. `Declared` had no callers; `Undeclare`'s only observable
effect beyond the tick check was suppressing one 10.7 ms tail block of a
removed track's last render — inaudible. Round 3 removed all of it
(~50 lines, including the "revive" path) with no observable behaviour
change.

### 2. Track and engine integration

- **Immutable track name.** `Track::Name()` returns an immutable
  `std::string` set in `Init()` and stored outside `settings_` — the
  engine's per-block dispatch loop reads it without taking
  `Track::mutex_` (a rename is a remove+add, per `SetupTracks` map
  semantics). `GetTrackName()` remains for the setup-path API.
- **Track caches its `Signal*` at `Init`** (`bus->Declare(name)`,
  idempotent) and publishes the post-FX, pre-fader, pre-mute block at
  the end of `ProcessLoop` — the audio path takes no registry lock.
- **Engine caches the master `Signal*`** (declared once at `Init`) and
  publishes the mixed block after the join phase. No per-block string
  construction.
- **Two-phase declare in `SetupTracks`:** all new track names are
  declared *before* any track in the batch is created, so an FX created
  in the same edit can resolve its source to a handle whatever the
  creation order.
- **Compressor source resolution** (`fx/fx_compressor.cc`):
  `ResolveSource()` runs only from `ReloadParams` (init / fast-update,
  never the render path) and caches the handle. While the handle is
  unresolved, `Render` re-looks it up every `kMissingRetryBlocks = 10`
  blocks. That retry is load-bearing: `FastUpdate` only re-resolves when
  the compressor's own `extra_` JSON changes, so a source track added in
  a later edit (compressor settings untouched) is picked up by the
  retry, not by the fast update. The cost is one registry lookup per 10
  blocks, and only while the source is missing.
- **Warn-once transition:** "sidechain source '…' is not available" is
  logged only on the transition into "missing" (latched flag, cleared on
  recovery) — never repeatedly from the audio path.
- **`FastUpdate` duration warning:** re-init takes `Track::mutex_` for
  its whole duration (the engine's join loop waits on it); `FastUpdate`
  logs a WARNING when it takes ≥ 1 ms, so a slow re-init (notably VST
  init) is directly measurable as the underrun cause it is.

### 3. Capture path (`inst/external.cc/.hh`) — the input overrun fix

The capture device delivers callbacks on the *driver's* schedule,
independent of our threads; every bound below exists because of that.

- **Requested period = `kBlockSize` (512), `periods = 2`.** Without this,
  miniaudio's ALSA backend defaults to a 100 ms period (4800 frames) —
  a single callback that would overflow any sane ring. `periods = 2`
  (21 ms) is the latency/robustness compromise asked of the driver.
  The driver may still negotiate a larger period, which is why the next
  three exist.
- **Ring = 16 blocks (170 ms).** Absorbs a negotiated period of up to 8
  blocks even if the `External` thread is briefly descheduled.
- **Multi-block drain (≤ `kMaxDrainBlocks = 8` per call).**
  `ProcessAudioInput` (External thread, once per engine block) drains as
  many full 512-frame blocks as are available. The bound keeps a burst
  of input from stalling the External thread, which also schedules MIDI
  with `sleep_until` precision.
- **`buffers_` cap = 16 blocks, drop oldest.** The consumer
  (`Render`, on the track thread) can stall (VM deschedule, re-init);
  without the cap the backlog would grow unboundedly at real-time rate.
  Dropping the oldest = skipping stale input rather than adding
  unbounded latency.

Diagnostics (capture device thread only, off the hot path when healthy):

- One-time setup log of the **negotiated** period ("Audio input callback
  delivers N frames per period (X ms)") — must show 512 / 10.7 ms for
  the fix to be in effect.
- `dropped_frames_` counter + rate-limited warning (≤ 1 / 2 s, only
  while input is actually being lost): "Audio input ring buffer is
  full: dropped … frames of input so far".

### 4. Output path (`audio/audio_output.cc/.hh`, `audio/pcm_stream.cc`)

- **Silence-fill on underrun (correctness, pre-existing).** The
  miniaudio device callback must return frames; when the ring holds less
  than the requested period, the remainder is filled with silence and the
  underrun is counted.
- **`underruns_` counter + `OnUnderrun`.** Called from the device thread
  *outside* `buffer_mutex_` (logging takes the global log lock; holding
  it under `buffer_mutex_` could block the engine's push). The warning is
  rate-limited to 1/s and only fires while underruns are actually
  happening — the steady state stays log-free.
- **1 s cap, drop oldest (`PushAudioBuffer`).** The engine's soft clock
  is an absolute grid: after a long stall it fast-forwards in a burst to
  catch up and can push many seconds of audio at once (see §5). The cap
  bounds that backlog (memory + the device thread's O(n) `erase` on the
  vector). In steady state the buffer holds a few blocks and this never
  triggers.
- **`PcmStream`** (HTTP/WebSocket streaming) is a fixed 10 s ring with
  wrap-around write position — bounded by construction, no explicit drop
  code.

### 5. Clocking

- **`steady_clock` for both soft clocks** (`Engine::Run`,
  `External::Run` and its `ScheduleMidiEvents`). The wall clock
  (`absl::Now` → `system_clock`) can be stepped by NTP or VM time
  synchronisation, which would stretch or skip the block schedule and
  underrun the output device / drop and duplicate MIDI events.
- **Absolute grid:** block N is due at `initial + N · block_duration` —
  a slow block is compensated by a fast-forward burst, never by
  drifting. Consequences after a long stall (all documented, none of
  them silent): the device underruns during the stall (counted, warned),
  the engine burst-renders the stale window (the 1 s output cap absorbs
  the backlog), and the External thread re-sends the stale window's MIDI
  events compressed (audible on the external synth).
- **Open design question — skip vs. catch-up.** If VM stalls become
  chronic, the alternative policy is to resync the grid to "now" when it
  is stale by more than a few blocks (skip the gap, DAW-style). That
  would make the output-ring cap unnecessary, but it changes what is
  heard after a stall (the skipped window is discarded instead of
  replayed late) and stale MIDI events would also need dropping in two
  places (the engine's `msgs_by_track_` swap and `External::midi_stack_`)
  to avoid a one-shot burst on the synth. Deliberate decision, not a
  simplification.

### 6. Diagnostics surface

- **`system.get_audio_underruns()`** (RT Python API, also printed by
  `system.info()`): underrun callbacks on the current output device
  instance (reset when the device is re-opened). A count that grows
  while listening confirms the crackles come from the engine not keeping
  up. Input-side drops are reported in the log instead.
- **Log lines and what they mean:**

| Line | Meaning | Action |
|------|---------|--------|
| `Audio input callback delivers N frames per period (X ms)` | Negotiated capture period (once at setup) | Expect 512 / 10.7 ms |
| `Audio input ring buffer is full: dropped …` | Capture period larger than the ring can absorb | Grow `ringbuffer_size` in `ConfigureAudioDevice` |
| `Audio output underrun: inserted …` | Engine did not push in time | Look for `FastUpdate` warnings / stalls |
| `FastUpdate of track '…' took … ms` | Re-init held the track lock (engine blocked meanwhile) | Reduce re-init cost (e.g. VST init); double-buffer as a follow-up |
| `Compressor FX '…': sidechain source '…' is not available` | `source='…'` does not match a publishing track (typo / removed) | Fix the name; audio passes through meanwhile |

All warnings are rate-limited and, except `FastUpdate` (setup thread),
never logged from a context that can't absorb a short log-lock hold.

## Rules for future changes to the hot path

- No registry locks per block (the signal bus registry is setup-only).
- No allocations or logging in steady state on the engine, track, device,
  or capture threads (rate-limited diagnostics only while failing).
- Any new per-block lock must be short-held (bounded by one
  `kBlockSize` copy) and per-signal, so different signals cannot
  contend.
- Soft clocks stay on `steady_clock`; new consumers of rendered blocks
  must bound their backlog (cap + drop oldest, or a fixed ring).

## Verification on the affected machine (VM + USB card)

1. Engine log, once at setup: `Audio input callback delivers N frames
   per period (X ms)` — with the fix N = 512 (10.7 ms). While playing:
   **no** `Audio input ring buffer is full` warnings.
2. While listening: `system.get_audio_underruns()` (or `system.info()`)
   stays at 0. If it grows: `FastUpdate of track '…' took … ms`
   warnings identify a slow re-init; without them, the VM is
   descheduling the engine (reduce host load, raise process priority
   (nice −20 / SCHED_FIFO with `CAP_SYS_NICE`), or move the USB card to
   a different host controller).

## Files

| File | Role |
|------|------|
| `cpp/core/signal_bus.hh/.cc` | Append-only registry + per-signal rings; `Declare`/`Find`/`Publish`/`Latest` |
| `cpp/core/track.hh/.cc` | Immutable `Name()`, cached `Signal*`, publish in `ProcessLoop`, `FastUpdate` duration warning |
| `cpp/core/engine.hh/.cc` | Master `Signal*` cache, two-phase declare, `steady_clock` grid, `GetAudioUnderruns()` |
| `cpp/fx/fx_compressor.cc/.hh` | `ResolveSource` (setup path), missing-source retry, warn-once |
| `cpp/inst/external.cc/.hh` | Capture period request, 16-block ring, multi-block drain, `buffers_` cap, drop diagnostics, `steady_clock` |
| `cpp/audio/audio_output.cc/.hh` | Underrun counter + `OnUnderrun`, 1 s output cap |
| `cpp/audio/pcm_stream.cc` | Fixed 10 s streaming ring (bounded by construction) |
| `cpp/bindings/rt.cc`, `py/soir/rt/system.py` | `get_audio_underruns()` RT API + `system.info()` line |
| `cpp/tests/core/signal_bus_test.cc` | Bus unit tests (append-only semantics, wrap-around, removed-signal behaviour) |
| `py/tests/integration/test_compressor.py` | Compressor / sidechain / ghost-trigger integration tests |
