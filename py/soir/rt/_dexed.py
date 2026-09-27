"""Internal Dexed patch DSL used by `tracks.mk_dexed`.

Dexed is a DX7-style FM synthesizer. A *patch* is a plain dict that
describes the whole sound:

```python
patch = {
    'algo': 3,
    'feedback': 0.2,
    'cutoff': 9000.0,
    'resonance': 0.0,
    'out': 0.8,
    'ops': [
        {'freq': 440.0, 'level': 0.9, 'env': swell_env(0.5)},
        {'freq': 880.0, 'mode': 'fm', 'level': 0.35, 'env': swell_env(0.3)},
    ],
}
```

`params(patch)` turns a patch into the normalized VST3 parameter dict
used by the plug-in. Named patches are available in `PRESETS`; the
preset list is documented in `tracks.mk_dexed`.

Value conventions
-----------------
- Operator `freq` is the frequency the operator produces while playing
  A4 (440 Hz). Use 440.0 for the note itself, 880.0 for one octave up,
  220.0 for one octave down, and odd ratios (e.g. 1320.0) for
  inharmonic partials.
- Envelope rates are 0.0 (slow) .. 1.0 (fast); levels are 0.0 (silent)
  .. 1.0 (full).
- `cutoff` is in Hz (20..20000, logarithmic).
- All other values are 0.0..1.0 unless stated otherwise.
"""

import math

# VST3 parameter names (as reported by Dexed).
_CUTOFF = "Cutoff"
_RESONANCE = "Resonance"
_OUTPUT = "Output"
_ALGORITHM = "ALGORITHM"
_FEEDBACK = "FEEDBACK"
_OSC_SYNC = "OSC KEY SYNC"
_TRANSPOSE = "TRANSPOSE"
_LFO_SPEED = "LFO SPEED"
_LFO_PM = "LFO PM DEPTH"
_LFO_AM = "LFO AM DEPTH"
_LFO_WAVE = "LFO WAVE"

_NUM_OPS = 6

# F COARSE: 31 stages; stage n>=1 is +n-1 semitones, stage 0 is -12 st.
# F FINE: 99 steps; step f adds log2(1 + 0.01*f) octaves (0..~0.122).

# A 4-stage DX envelope: (rate, level) pairs for stages 1..4.
Env = dict[int, tuple[float, float]]

# One FM operator, as produced by `op()`.
OpSpec = dict[str, object]

# A Dexed patch spec (see module docstring).
PatchSpec = dict[str, object]

# The preset used by `tracks.mk_dexed` when no patch is given.
DEFAULT_PATCH = "warm_pad"


def _clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def _step(value: float, steps: int) -> float:
    """Normalize a 0..1 value onto a `steps`-wide staircase."""
    return _clamp(round(value * (steps - 1)) / (steps - 1), 0.0, 1.0)


def _num(value: object, default: float) -> float:
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return float(value)
    return default


def _int(value: object, default: int) -> int:
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return int(value)
    return default


def _freq_to_coarse_fine(freq_hz: float) -> tuple[int, int]:
    """Map an A4-referenced frequency to (F COARSE, F FINE) DX values."""
    semis = 12.0 * math.log2(_clamp(freq_hz, 1.0, 20000.0) / 440.0)
    coarse = int(round(semis)) + 1
    cents = (semis - (coarse - 1)) * 100.0
    if cents <= 0.0:
        coarse -= 1
        cents += 100.0
    coarse = int(_clamp(coarse, 0, 30))
    fine = int(
        _clamp(round(99.0 * math.log(1.0 + cents / 100.0) / math.log(1.99)), 0, 99)
    )
    return coarse, fine


def _hz_to_cutoff(hz: float) -> float:
    return _clamp(
        math.log2(_clamp(hz, 20.0, 20000.0) / 20.0) / math.log2(20000.0 / 20.0),
        0.0,
        1.0,
    )


def pad_env(
    attack: float, sustain: float, peak: float = 1.0, release: float = 0.5
) -> Env:
    """Slow-breathing envelope for sustained pads.

    Args:
        attack: The attack rate in [0.0, 1.0].
        sustain: The sustain level in [0.0, 1.0].
        peak: The peak level in [0.0, 1.0]. Defaults to 1.0.
        release: The release rate in [0.0, 1.0]. Defaults to 0.5.

    Returns:
        An envelope spec for use in an operator.
    """
    return {
        1: (attack, peak),
        2: (attack, sustain),
        3: (0.05, sustain),
        4: (release, 0.0),
    }


def swell_env(sustain: float, peak: float = 1.0, release: float = 0.4) -> Env:
    """Very slow attack for transient-free swells; sustain must be long
    enough for the attack to develop.

    Args:
        sustain: The sustain level in [0.0, 1.0].
        peak: The peak level in [0.0, 1.0]. Defaults to 1.0.
        release: The release rate in [0.0, 1.0]. Defaults to 0.4.

    Returns:
        An envelope spec for use in an operator.
    """
    return {
        1: (0.1, peak),
        2: (0.12, sustain),
        3: (0.05, sustain),
        4: (release, 0.0),
    }


def op(
    freq: float,
    level: float = 0.8,
    on: bool = True,
    mode: str = "same",
    env: Env | None = None,
    detune: int = 7,
    rate_scaling: int = 0,
    key_velocity: int = 3,
) -> OpSpec:
    """Describe one FM operator.

    Args:
        freq: The frequency at A4, in Hz (440.0 = the note itself).
        level: The operator output level in [0.0, 1.0]. Defaults to 0.8.
        on: Whether the operator is switched in. Defaults to True.
        mode: 'same' (additive stage) or 'fm' (modulating stage).
        env: Envelope stages; defaults to a soft pad envelope.
        detune: DX detune stage in [0, 13] (7 is center). Defaults to 7.
        rate_scaling: Key scaling of the envelope in [0, 7]. Defaults to 0.
        key_velocity: Velocity sensitivity in [0, 7] (3 is nominal). Defaults to 3.

    Returns:
        An operator spec for use in a patch.
    """
    if env is None:
        env = swell_env(0.5)
    return {
        "freq": freq,
        "level": level,
        "on": on,
        "mode": mode,
        "env": env,
        "detune": detune,
        "rate_scaling": rate_scaling,
        "key_velocity": key_velocity,
    }


def params(patch: PatchSpec) -> dict[str, float]:
    """Convert a patch spec into Dexed VST3 parameters.

    All six operators are always emitted: operators not present in the
    spec are switched off, so a patch always defines the whole sound.

    Args:
        patch: Patch dict; keys:
            - `algo` (int 1..15, default 1)
            - `feedback` (0..1, default 0.0)
            - `cutoff` (Hz, default 12000)
            - `resonance` (0..1, default 0.0)
            - `out` (0..1, default 0.8)
            - `transpose` (semitones -24..24, default 0)
            - `lfo` (dict with optional `speed`, `pm`, `am`, `wave`)
            - `ops` (list of op specs, 1..6)
            - any other key is passed through as a raw normalized
              VST3 parameter (exact Dexed title).

    Returns:
        The normalized {parameter title: value} dict for Dexed.

    Raises:
        ValueError: If the patch has no operators, more than six
            operators, or a non-dict operator.
    """
    algo = _clamp(_num(patch.get("algo", 1), 1.0), 1.0, 15.0)
    out: dict[str, float] = {
        _ALGORITHM: (algo - 1.0) / 30.0,
        _FEEDBACK: _step(_num(patch.get("feedback", 0.0), 0.0), 7),
        _CUTOFF: _hz_to_cutoff(_num(patch.get("cutoff", 12000.0), 12000.0)),
        _RESONANCE: _clamp(_num(patch.get("resonance", 0.0), 0.0), 0.0, 1.0),
        _OUTPUT: _clamp(_num(patch.get("out", 0.8), 0.8), 0.0, 1.0),
        _OSC_SYNC: 0.0,
        _LFO_SPEED: 0.0,
        _LFO_PM: 0.0,
        _LFO_AM: 0.0,
        _LFO_WAVE: 0.0,
    }
    transpose = _int(patch.get("transpose", 0), 0)
    out[_TRANSPOSE] = _clamp((transpose + 24) / 48.0, 0.0, 1.0)

    lfo = patch.get("lfo")
    if isinstance(lfo, dict):
        out[_LFO_SPEED] = _clamp(_num(lfo.get("speed", 0.0), 0.0), 0.0, 1.0)
        out[_LFO_PM] = _clamp(_num(lfo.get("pm", 0.0), 0.0), 0.0, 1.0)
        out[_LFO_AM] = _clamp(_num(lfo.get("am", 0.0), 0.0), 0.0, 1.0)
        out[_LFO_WAVE] = _step(_num(lfo.get("wave", 0), 0.0), 5)

    ops = patch.get("ops")
    if not isinstance(ops, list) or not ops:
        raise ValueError("patch needs at least one operator in 'ops'")
    if len(ops) > _NUM_OPS:
        raise ValueError(f"Dexed has at most {_NUM_OPS} operators")

    for i in range(1, _NUM_OPS + 1):
        prefix = f"OP{i}"
        op_spec = ops[i - 1] if i <= len(ops) else None
        if not isinstance(op_spec, dict):
            if op_spec is not None:
                raise ValueError(f"operator {i} must be a dict")
            out[f"{prefix} SWITCH"] = 0.0
            continue

        coarse, fine = _freq_to_coarse_fine(_num(op_spec.get("freq", 440.0), 440.0))
        out[f"{prefix} F COARSE"] = coarse / 30.0
        out[f"{prefix} F FINE"] = fine / 99.0
        out[f"{prefix} OUTPUT LEVEL"] = _clamp(
            _num(op_spec.get("level", 0.8), 0.8), 0.0, 1.0
        )
        out[f"{prefix} SWITCH"] = 1.0 if op_spec.get("on", True) else 0.0
        mode = op_spec.get("mode", "same")
        out[f"{prefix} MODE"] = 1.0 if mode == "fm" else 0.0
        out[f"{prefix} OSC DETUNE"] = _int(op_spec.get("detune", 7), 7) / 13.0
        out[f"{prefix} RATE SCALING"] = _int(op_spec.get("rate_scaling", 0), 0) / 7.0
        out[f"{prefix} KEY VELOCITY"] = _int(op_spec.get("key_velocity", 3), 3) / 7.0
        env = op_spec.get("env")
        if not isinstance(env, dict):
            env = swell_env(0.5)
        for stage in range(1, 5):
            rate, level = env.get(stage, (0.05, 0.0))
            out[f"{prefix} EG RATE {stage}"] = _clamp(_num(rate, 0.05), 0.0, 1.0)
            out[f"{prefix} EG LEVEL {stage}"] = _clamp(_num(level, 0.0), 0.0, 1.0)

    for key, value in patch.items():
        if key in (
            "algo",
            "feedback",
            "cutoff",
            "resonance",
            "out",
            "transpose",
            "lfo",
            "ops",
        ):
            continue
        if isinstance(value, (int, float)) and not isinstance(value, bool):
            out[str(key)] = float(value)

    return out


PRESETS: dict[str, PatchSpec] = {
    "warm_pad": {
        "algo": 2,
        "feedback": 0.05,
        "cutoff": 7000.0,
        "resonance": 0.0,
        "out": 0.8,
        "lfo": {"speed": 0.1, "am": 0.08, "wave": 0},
        "ops": [
            op(440.0, level=0.9, env=swell_env(0.6, release=0.5)),
            op(880.0, level=0.25, mode="fm", env=swell_env(0.35)),
        ],
    },
    "vangelis_pad": {
        "algo": 3,
        "feedback": 0.1,
        "cutoff": 8500.0,
        "resonance": 0.0,
        "out": 0.75,
        "lfo": {"speed": 0.15, "am": 0.12, "wave": 0},
        "ops": [
            op(440.0, level=0.95, env=swell_env(0.55, release=0.6)),
            op(880.0, level=0.3, mode="fm", env=swell_env(0.4)),
            op(1760.0, level=0.12, mode="fm", env=swell_env(0.25)),
        ],
    },
    "dark_bass": {
        "algo": 1,
        "feedback": 0.15,
        "cutoff": 2200.0,
        "resonance": 0.15,
        "out": 0.85,
        "ops": [
            op(220.0, level=0.9, env=swell_env(0.75, release=0.5), key_velocity=2),
            op(440.0, level=0.5, mode="fm", env=swell_env(0.4), key_velocity=2),
        ],
    },
    "glass_lead": {
        "algo": 2,
        "feedback": 0.2,
        "cutoff": 11000.0,
        "resonance": 0.1,
        "out": 0.75,
        "lfo": {"speed": 0.25, "am": 0.1, "wave": 0},
        "ops": [
            op(440.0, level=0.9, env=swell_env(0.7, release=0.4)),
            op(880.0, level=0.25, mode="fm", env=swell_env(0.35)),
        ],
    },
    "soft_bell": {
        "algo": 5,
        "feedback": 0.0,
        "cutoff": 14000.0,
        "resonance": 0.0,
        "out": 0.8,
        "ops": [
            op(440.0, level=0.85, env=swell_env(0.55)),
            op(880.0, level=0.35, mode="fm", env=swell_env(0.35)),
            op(1320.0, level=0.2, mode="fm", env=swell_env(0.25)),
        ],
    },
    "sub_drone": {
        "algo": 1,
        "feedback": 0.0,
        "cutoff": 400.0,
        "resonance": 0.0,
        "out": 0.8,
        "lfo": {"speed": 0.08, "am": 0.2, "wave": 0},
        "ops": [
            op(110.0, level=0.9, env=swell_env(1.0, release=0.4)),
        ],
    },
    "wide_drone": {
        "algo": 1,
        "feedback": 0.05,
        "cutoff": 3000.0,
        "resonance": 0.0,
        "out": 0.8,
        "lfo": {"speed": 0.06, "am": 0.12, "wave": 0},
        "ops": [
            op(440.0, level=0.7, detune=4, env=pad_env(0.4, 0.8)),
            op(440.0, level=0.7, detune=10, env=pad_env(0.4, 0.8)),
        ],
    },
    "pluck": {
        "algo": 1,
        "feedback": 0.1,
        "cutoff": 4000.0,
        "resonance": 0.0,
        "out": 0.85,
        "ops": [
            op(
                440.0,
                level=0.9,
                rate_scaling=5,
                env={1: (1.0, 1.0), 2: (0.55, 0.08), 3: (0.05, 0.08), 4: (0.45, 0.0)},
            ),
            op(
                880.0,
                level=0.45,
                mode="fm",
                env={1: (0.9, 1.0), 2: (0.5, 0.0), 3: (0.05, 0.0), 4: (0.4, 0.0)},
            ),
        ],
    },
    "bell": {
        "algo": 4,
        "feedback": 0.0,
        "cutoff": 14000.0,
        "resonance": 0.0,
        "out": 0.8,
        "ops": [
            op(
                440.0,
                level=0.9,
                env={1: (1.0, 1.0), 2: (0.5, 0.05), 3: (0.05, 0.05), 4: (0.35, 0.0)},
            ),
            op(
                880.0,
                level=0.5,
                mode="fm",
                env={1: (0.8, 1.0), 2: (0.45, 0.0), 3: (0.05, 0.0), 4: (0.4, 0.0)},
            ),
            op(
                1320.0,
                level=0.25,
                mode="fm",
                env={1: (0.7, 1.0), 2: (0.35, 0.0), 3: (0.05, 0.0), 4: (0.4, 0.0)},
            ),
        ],
    },
    "brass_stab": {
        "algo": 3,
        "feedback": 0.15,
        "cutoff": 6000.0,
        "resonance": 0.15,
        "out": 0.8,
        "lfo": {"speed": 0.3, "pm": 0.15, "am": 0.05, "wave": 0},
        "ops": [
            op(
                440.0,
                level=0.9,
                env={1: (0.9, 1.0), 2: (0.3, 0.7), 3: (0.05, 0.7), 4: (0.4, 0.0)},
            ),
            op(
                880.0,
                level=0.35,
                mode="fm",
                env={1: (0.8, 1.0), 2: (0.35, 0.2), 3: (0.05, 0.2), 4: (0.35, 0.0)},
            ),
        ],
    },
    "singing_lead": {
        "algo": 2,
        "feedback": 0.1,
        "cutoff": 9000.0,
        "resonance": 0.05,
        "out": 0.75,
        "lfo": {"speed": 0.2, "am": 0.15, "wave": 0},
        "ops": [
            op(
                440.0,
                level=0.9,
                env={1: (0.7, 1.0), 2: (0.25, 0.75), 3: (0.05, 0.75), 4: (0.4, 0.0)},
            ),
            op(
                880.0,
                level=0.3,
                mode="fm",
                env={1: (0.6, 1.0), 2: (0.3, 0.4), 3: (0.05, 0.4), 4: (0.35, 0.0)},
            ),
        ],
    },
    "metallic": {
        "algo": 5,
        "feedback": 0.5,
        "cutoff": 12000.0,
        "resonance": 0.25,
        "out": 0.75,
        "ops": [
            op(
                440.0,
                level=0.85,
                env={1: (0.95, 1.0), 2: (0.4, 0.15), 3: (0.05, 0.15), 4: (0.5, 0.0)},
            ),
            op(
                1320.0,
                level=0.4,
                mode="fm",
                env={1: (0.85, 1.0), 2: (0.4, 0.0), 3: (0.05, 0.0), 4: (0.45, 0.0)},
            ),
        ],
    },
}
