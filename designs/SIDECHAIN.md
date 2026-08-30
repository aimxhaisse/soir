# Sidechain Compression (Compression Keyed on Another Track)

## Overview

Add a **compressor** effect that can be *keyed* on the output of another
track (classic sidechain compression, e.g. the kick track ducking the pads),
on the master mix, or on its own input (plain in-line compressor).

```python
tracks.setup({
    'kick': tracks.mk_sampler(),
    'pads': tracks.mk_sampler(fxs={
        'duck': fx.mk_compressor(source='kick', ratio=8.0, release=0.3),
    }),
})
```

A sidechain source can also be **inaudible**: the source signal is tapped
pre-mute, so a muted (or zero-volume) track keeps driving the effect while
never being heard — an inaudible "ghost" trigger track:

```python
tracks.setup({
    'ghost': tracks.mk_sampler(muted=True),  # inaudible trigger
    'pads': tracks.mk_sampler(fxs={
        'wobble': fx.mk_compressor(source='ghost', ratio=12.0, release=0.15),
    }),
})
```

The core difficulty is architectural, not DSP: **tracks render in parallel on
separate threads**, so track B's FX cannot see track A's audio for the same
block. We solve this with a small inter-track **signal bus** that exposes each
track's rendered output with a deterministic one-block delay (~10.7 ms),
which is inaudible for sidechain use and keeps the existing thread model
untouched.

## The architectural challenge

The engine loop (`Engine::Run`) processes each 512-sample block (~10.7 ms at
48 kHz) as follows:

1. Dispatch `Track::RenderAsync(tick, events)` to **every** track — each
   track renders `instrument → fx stack` in its own thread.
2. `Track::Join(buffer)` — the engine thread waits for each track to finish
   and mixes its post-FX output (volume/pan/mute) into the master buffer.
3. Push the master buffer to consumers.

There is no signal path from one track's buffer to another track's FX stack.
Any cross-track effect needs one.

### Alternatives considered

| Option | Delay | Complexity | Verdict |
|--------|-------|------------|---------|
| **A. Signal bus, one-block delayed source** | 1 block (10.7 ms) | Low — add a registry, publish outputs | **Chosen** |
| B. Render key tracks synchronously first in the engine thread | 0 | High — engine must derive key dependency graph from FX settings, detect cycles, special-case track threading | Future upgrade if ever needed |
| C. Python-computed key (read `levels` of the key track in the loop, drive a gain knob) | 1 block + knob latency (10 ms) | None (works today) | Crude workaround; per-block (10 ms) Python-side envelope follower is too slow for musical ducking. Knobs remain the *parameter* automation path (see below) |

One block of source delay is standard practice territory: hardware sidechains
typically add more, and the musically relevant motion (attack of a kick,
decay of a duck) is far slower than 10.7 ms. Deterministic one-block delay is
preferred over a 0–1 block jitter (which would happen if we published inside
`Join`, since tracks finish out of order).

## C++ design

### 1. Signal bus (`core/signal_bus.hh/.cc`, new)

A registry of named per-track (plus `master`) audio outputs, double-buffered
with a small ring per name:

```cpp
// core/signal_bus.hh
namespace soir {

struct Block {
  SampleTick tick;  // tick of the first sample in this block
  const float* left;
  const float* right;
};

class SignalBus {
 public:
  // Pre-allocate slots for a signal name (called from SetupTracks for
  // every track name, and once for kMasterSignal).
  void Declare(const std::string& name);
  void Undeclare(const std::string& name);

  // Publish the block starting at `tick`. Called by the track thread at
  // the end of Track::ProcessLoop for track signals, and by the engine
  // thread after the join phase for the master signal. Slots are indexed
  // by (tick / kBlockSize) % kDepth.
  void Publish(const std::string& name, SampleTick tick,
               const float* left, const float* right, int size);

  // Read the block at (tick - kBlockSize), i.e. the most recent fully
  // rendered block, or nullptr if that name has never published a block
  // at that position (source track not present yet / just created).
  const Block* Latest(const std::string& name, SampleTick tick) const;

 private:
  static constexpr int kDepth = 4;
  // name -> ring of kDepth slots, each a pair of std::vector<float>
  std::mutex mutex_;  // protects the map only, never the sample data
  std::map<std::string, std::unique_ptr<Signal>> signals_;
};

static constexpr std::string_view kMasterSignal = "soir_internal_master";

}  // namespace soir
```

Key properties:

- **Deterministic 1-block delay.** A reader rendering block N always reads
  the key's block N-1. This is guaranteed by the existing engine
  synchronisation: the engine only dispatches block N+1 to any track after
  it has joined (observed completion of) block N for *all* tracks, and
  publishes the master after the join loop. `kDepth = 4` guarantees a
  writer never overwrites a slot a reader may still be consuming.
- **No locks on the sample path.** The map mutex is taken once per
  `Latest()`/`Publish()` for name resolution; the float data is written
  exactly once per block per slot (happens-before via the
  cv/mutex handoff of the engine loop), so readers read stable memory
  without copying.
- **Missing source = silence.** If the source track doesn't exist (typo, or
  removed), `Latest()` returns `nullptr` and the compressor passes audio
  through unchanged (plus a rate-limited warning log). A live coding
  environment should never kill a track because its source disappeared.
- **Memory.** 4 slots × 512 samples × 2 ch × 4 B = 16 KB per signal name —
  negligible.

### 2. What gets published, and when

- **Track signals.** Published by the track thread in `Track::ProcessLoop`
  right after `level_meter_.Process(...)`, i.e. the **post-FX, pre-fader,
  pre-mute** buffer — the track's rendered output, unconditionally.
  The tap is deliberately placed *before* mute and fader:
  - **Mute is a mixing decision, the source is a routing decision** — like
    a hardware key input, an independent path that muting the audible
    signal doesn't touch. `muted_` and `volume_` never affect the source
    signal; no extra state (e.g. atomic muted mirror) is needed, the
    track just always publishes its rendered buffer.
  - This enables the **ghost trigger pattern**: a track created (or set)
    with `muted=True` — or `volume=0` — renders in full (muted tracks
    already run their instrument + FX, mute is only checked at mix time
    in `Join`), drives its sidechain consumers, and is never heard. Its
    level meter keeps working, so the user can verify the trigger is
    firing.
  - Consequence: muting a source track does **not** stop the ducking. To
    stop it, stop triggering the source track, or automate `wet` to 0
    (dry/wet blend) via a `Control` — the live-coding-friendly on/off.
  The track needs the bus pointer in `Track::Init` (the engine already
  passes `sample_manager_`, `controls_`, `vst_host_` there).
- **Master signal.** Published by the engine thread in `Run()` right after
  the join loop (before `master_meter_.Process`), from the mixed buffer.
  Enables `source="master"` (duck a track when the whole mix gets loud)
  with no new master-FX infrastructure.

Every track name is therefore automatically a sidechain source — no user
declaration step.

### 3. Compressor DSP (`dsp/compressor.hh/.cc`, new)

Pure DSP struct, testable standalone (same pattern as `dsp::Chorus`,
`dsp::Reverb`):

```cpp
namespace soir::dsp {

struct Compressor {
  struct Parameters {
    float threshold_ = 0.25f;  // linear amplitude [0.0, 1.0]
    float ratio_ = 4.0f;       // [1.0, 20.0]
    float attack_ = 0.005f;    // seconds  [0.0005, 0.5]
    float release_ = 0.15f;    // seconds  [0.01, 2.0]
    float knee_ = 6.0f;        // dB, total width of the soft knee [0.0, 12.0]
    float makeup_ = 1.0f;      // gain     [0.0, 4.0]
    float wet_ = 1.0f;         // dry/wet  [0.0, 1.0]
  };

  void Reset();
  void FastUpdate(const Parameters& params);  // recompute attack/release alphas
  // Feedforward: envelope follows `key`, gain is applied to `in`.
  std::pair<float, float> Process(float key_l, float key_r,
                                  float in_l, float in_r);
  float GainReduction() const;  // dB, for metering/debug/tests
};

}  // namespace soir::dsp
```

Per-sample algorithm (mono envelope, linked stereo gain — standard for
musical sidechain use):

```
# Envelope follower (linear domain)
key  = max(|key_l|, |key_r|)
env  = env + (key - env) * (key > env ? attack_alpha : release_alpha)
alpha_t = 1 - exp(-1 / (t * kSampleRate))

# Gain reduction (dB domain, parabolic soft knee)
in_db = 20 * log10(max(env, 1e-6))
t_db  = 20 * log10(max(threshold, 1e-6))

if knee <= 0:                            # hard knee
    y_db = in_db <= t_db ? in_db : t_db + (in_db - t_db) / ratio
else:                                    # band [t_db - knee/2, t_db + knee/2]
    if in_db <= t_db - knee/2:
        y_db = in_db
    elif in_db >= t_db + knee/2:
        y_db = t_db + (in_db - t_db) / ratio
    else:                                # parabolic blend (C1-continuous)
        y_db = in_db + (1/ratio - 1) * (in_db - (t_db - knee/2))^2 / (2 * knee)

gain = 10^((y_db - in_db) / 20)     # 1.0 below the band, < 1 inside/above

out = in * (gain * makeup) * wet + in * (1 - wet)
```

Notes:

- **Soft knee, knob-able.** `knee` is the total width of the transition
  band in dB, centered on the threshold (default 6.0 → ±3 dB; 0.0 gives a
  hard knee, special-cased to avoid the division by zero). Units follow
  the precedent set by `attack`/`release` (natural units, not 0–1);
  `threshold` stays linear 0–1 like the other FX parameters.
- **Dry/wet blend** (`wet`) so the user can parallel-blend the compression,
  matching the reverb FX convention of putting the blend in `extra`
  (the `Fx::Settings::mix_` field is parsed by the bindings but currently
  consumed by no FX — we leave it alone).
- All numeric parameters are exposed through `Parameter` (below), so any of
  them can be a knob and is live-automatable.

### 4. Compressor FX (`fx/fx_compressor.hh/.cc`, new)

The wiring layer, following `fx_echo`/`fx_reverb` structure:

- `Init`/`CanFastUpdate`/`FastUpdate` — JSON `extra_` via
  `Parameter::FromJSON` for `threshold`, `ratio`, `attack`, `release`,
  `knee`, `makeup`, `wet`. The `source` field is a plain string
  (`"self"` (default) | track name | `"master"`); changing it is a
  `FastUpdate`-safe no-op on DSP state (only the lookup name changes), so
  it round-trips through `layout()`/`setup()` idempotently.
- `Render(tick, buffer, events)` takes `mutex_` like the other FX, then:
  - **`source == "self"`** — plain feedforward compressor, *sample-accurate*:
    the envelope follows the current input sample, so there is no extra
    delay. `out[i] = comp.Process(in[i], in[i])` in place.
  - **`source` = track name or `"master"`** — the FX asks
    `bus_->Latest(source, tick)` for the 1-block-delayed block and runs
    the same DSP with the source samples:
    `out[i] = comp.Process(src_l[i], src_r[i], in[i], in[i])`. If the
    lookup returns `nullptr`, pass through untouched.
- Constructor takes `Controls*` and `SignalBus*` (the bus is created in
  `Engine::Init` and handed to `FxStack`, mirroring how `VstHost*` flows
  `Engine → Track → FxStack → Fx`).

Self-keying vs external keying uses two envelope sources but one DSP core;
the "self" path is what makes this a *general* compressor rather than only
a sidechain device.

### 5. Plumbing changes

| File | Change |
|------|--------|
| `core/signal_bus.hh/.cc` | New — the bus |
| `dsp/compressor.hh/.cc` | New — pure DSP |
| `fx/fx_compressor.hh/.cc` | New — FX wiring |
| `fx/fx.hh` | `Type` enum: add `COMPRESSOR` |
| `fx/fx_stack.hh/.cc` | Constructor takes `SignalBus*`; `Init` switch gets a `COMPRESSOR` case |
| `core/track.hh/.cc` | `Init` takes `SignalBus*`; `ProcessLoop` publishes its post-FX block (unconditionally, pre-mute) |
| `core/engine.hh/.cc` | Owns `SignalBus`; `Run()` publishes the master block after join; `SetupTracks` declares/undeclares track names |
| `bindings/rt.cc` | `"compressor"` ↔ `fx::Type::COMPRESSOR` in both `setup_tracks_` and `get_tracks_` |
| `CMakeLists.txt` | Add the four new files (alphabetical order) |

### 6. Threading summary

- Writers: one thread per signal (its track thread, or the engine thread
  for master). Each slot is written exactly once per block.
- Readers: any FX in any track thread, only ever the *previous* block's
  slot.
- Synchronisation: the existing engine cv/mutex hand-off already orders
  "all of block N published" before "any of block N+1 rendered". No new
  locks on the sample path.

## Python design

### API

`py/soir/rt/fx.py`:

```python
def mk_compressor(
    source: str | None = None,
    threshold: float | Control = 0.25,
    ratio: float | Control = 4.0,
    attack: float | Control = 0.005,
    release: float | Control = 0.15,
    knee: float | Control = 6.0,
    makeup: float | Control = 1.0,
    wet: float | Control = 1.0,
) -> Fx:
    """Creates a new Compressor FX.

    Args:
        source: The signal used to compute the gain reduction. A track
                name for sidechain compression, "master" for the master
                mix, or None for the compressor's own input (plain
                in-line compression). Defaults to None.
        threshold: The threshold in the [0.0, 1.0] range.
        ratio: The compression ratio (1.0 = no compression).
        attack: The attack time in seconds.
        release: The release time in seconds.
        knee: The width of the soft knee around the threshold in dB
              (0.0 = hard knee). Defaults to 6.0.
        makeup: The makeup gain.
        wet: The dry/wet blend in the [0.0, 1.0] range.
    """
```

`source=None` serialises to `"self"` in the `extra` JSON. Everything else
flows through the existing `serialize_parameters` mechanism, so every
numeric parameter accepts a `Control`.

**Naming.** The parameter is called `source` rather than `key`: `key` is a
loaded idiom in Python (sort keys, dict keys) and in `tracks.setup` the
dict key *is* the track name, so `key=` would clash with it in the same
expression. In audio terms `source` is the compressor's *key input*.
Layering: the Python/JSON/FX layer uses `source`; the `dsp::Compressor`
sample-level inputs keep the domain term (`key_l`, `key_r`); the bus deals
in named *signals*.

### Usage

```python
# Sidechain: pads duck when the kick hits
tracks.setup({
    'kick': tracks.mk_sampler(),
    'pads': tracks.mk_sampler(fxs={
        'duck': fx.mk_compressor(source='kick', ratio=8.0,
                                 attack=0.001, release=0.3),
    }),
})

# Plain in-line compressor
tracks.setup({'bass': tracks.mk_sampler(fxs={
    'comp': fx.mk_compressor(threshold=0.3, ratio=3.0),
})})

# Duck everything when the whole mix gets loud
tracks.setup({'pads': tracks.mk_sampler(fxs={
    'duck': fx.mk_compressor(source='master', ratio=2.0),
})})
```

### Ghost trigger track (muted source)

The source tap is pre-mute, so a track can drive a sidechain without being
heard. This is the standard "pads wobble on a kick you don't hear" setup:

```python
tracks.setup({
    # Inaudible trigger: renders and drives the sidechain, never mixed
    # to the master.
    'ghost': tracks.mk_sampler(muted=True),
    'pads': tracks.mk_sampler(fxs={
        'wobble': fx.mk_compressor(source='ghost', ratio=12.0,
                                   attack=0.001, release=0.15),
    }),
})

s = sampler.new('808')

@loop(track='ghost', beats=4)
def trigger():
    for i in range(4):
        s.play('kick')
        sleep(1)
```

Notes:

- `volume=0` on the source track gives the same "inaudible but driving"
  behaviour (the tap is pre-fader too). Muting or zeroing the source track
  does **not** stop the ducking — that is intentional (mute is a mixing
  decision, the source is a routing decision).
- To turn the ducking off live (from inside a loop, where
  `tracks.setup()` is not allowed), bind `wet` to a `Control` and set it
  to 0: `wet=0` is a pure dry pass-through.
- The ghost track's level meter still reports its output, so you can
  confirm the trigger is firing while it stays silent.

### Live automation

Because `tracks.setup()` cannot be called inside a loop, the live knob path
is the existing `Control` mechanism: bind a `Control` at setup time, mutate
it from live code afterwards. The engine samples knobs at 100 Hz and
interpolates, so the change is smooth:

```python
from soir.rt import ctrls

# global scope, created once
ctrls.mk_val('duck_ratio', 4.0)
tracks.setup({
    'kick': tracks.mk_sampler(),
    'pads': tracks.mk_sampler(fxs={
        'duck': fx.mk_compressor(source='kick',
                                 ratio=ctrl('duck_ratio')),
    }),
})

@live()
def set_duck(ratio: float):
    ctrl('duck_ratio').set(ratio)
```

(`ctrl()` / the registry lookup resolves the named control; the C++ side
resolves the control name inside the FX's `Parameter`s.)

### Doc site

`fx.mk_compressor` picks up the documentation site automatically through the
existing docstring extraction (`py/soir/www`).

## Edge cases

| Case | Behaviour |
|------|-----------|
| Source track doesn't exist (typo / removed later) | Pass-through + rate-limited warning log. No setup failure. |
| Source track is muted (or `volume=0`) | Source signal is **unaffected** (pre-mute/pre-fader tap) → ducking continues. This is the ghost trigger pattern; stop the ducking by stopping the source track's triggers or automating `wet` to 0. |
| Source track created after the compressor | `Latest()` returns `nullptr` until the source's first block publishes → clean transition from no-ducking to ducking. |
| Chained sidechains (A keys B, B keys C) | Each hop adds 1 block; fine. |
| `source="self"` with other FX in the same chain | The envelope uses the buffer *as received by the compressor* (post any preceding FX), which is the correct "input" semantics. |
| Knob changes on compressor params | Interpolated by `Parameter`, no glitches (same as all other FX). |
| `ratio < 1` (expander territory) | Clamped to 1.0 by the parameter range. Expanders are out of scope. |

## Testing

- **C++ unit tests** (`cpp/tests/dsp/compressor_test.cc`): envelope
  follower convergence, no gain reduction below the knee band, gain
  reduction above the band matches the ratio math, soft-knee continuity at
  the band edges (plus the `knee=0` hard-knee special case), attack vs
  release timing, makeup, dry/wet blend, silent source → unity.
- **C++ unit tests** (`cpp/tests/core/` or similar): `SignalBus` —
  publish/read across slots, missing name → nullptr, wrap-around at
  `kDepth`.
- **Integration** (`py/tests/integration/test_compressor.py` or an addition
  to `test_tracks.py`):
  - `setup`/`layout` round-trip of a compressor (idempotent, params
    preserved, `source` string round-trips).
  - Audible behaviour: record the master with (a) a kick track + pad track
    with a `source='kick'` compressor vs (b) the same pad track without
    the compressor → RMS of the pad content is measurably lower in (a)
    while the kick is playing, and unchanged when the kick is silent.
  - Ghost pattern: source track set to `muted=True` → still ducks (the
    source tap is pre-mute), while the master contains no source-track
    content.

## Implementation phases

1. **Bus** — `SignalBus`, engine/track publish hooks, unit tests. No
   behaviour change yet (publishes are unused).
2. **Compressor** — `dsp::Compressor` (including the soft-knee gain math) +
   `fx::Compressor` + `Type` enum + `FxStack` case + bindings + CMake.
   `source="self"` works here.
3. **Sidechain** — `source="<track>"` / `source="master"` paths (bus reads
   in `Render`), Python `fx.mk_compressor`, docstrings, integration tests.
4. **Polish** — GR metering in `Levels`/cast UI (optional), docs/examples
   on the www site.

## Decisions (resolved)

- **Parameter name**: `source` (see the Naming note in the Python
  section).
- **Tap point**: post-FX, pre-fader, pre-mute — what the source track
  renders; mute/fader never affect it (ghost trigger pattern).
- **Knee**: parabolic soft knee, knob-able `knee` in dB, default 6.0
  (0.0 = hard knee).
- **Source latency**: deterministic 1 block (10.7 ms); no zero-delay path
  in v1 (see future work below).

## Future work (not v1)

1. **Mute-aware source tap** (`source='kick:audible'`) — publish zeros
   when the source track is muted (atomic muted mirror updated in
   `FastUpdate`), for users who want muting the source to also stop the
   ducking.
2. **Pre-FX source tap** (`source='kick:pre'`) — drive the duck from the
   dry instrument output, before the source track's own FX (e.g. don't
   let its reverb tail trigger the duck). Requires publishing a second
   signal per track (one extra pre-FX tap in `ProcessLoop`).
3. **Master-bus compressor.** `source="master"` covers "duck a track on
   the mix", but compressing the mix itself would need a master FX chain
   (new engine feature).
4. **Zero-delay source (B-lite).** An opt-in track flag (e.g.
   `priority=True`) marking tracks to be rendered synchronously in the
   engine thread before the parallel dispatch; they publish the *current*
   block, so FX keyed on them see zero delay. No dependency analysis or
   cycle detection — the user declares what's a key source. Small change:
   one flag in `Track::Settings` + one ordering tweak in `Run()`. A full
   automatic version (derive the key graph from FX settings, detect
   cycles) is considerably more complex and only warranted if B-lite
   proves insufficient.
5. **More keyed FX.** The bus makes sidechain *gating*, *ducking*
   (simpler attenuator), or key-modulated filters trivial to add later as
   new FX reading `Latest()`.
6. **Live source switching.** `source=['a', 'b']` plus a selector knob to
   retarget the envelope at 100 Hz.
