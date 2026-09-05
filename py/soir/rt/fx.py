"""
The **fx** module contains DSP effects that can be attached to tracks.

The `fx` module provides a set of audio effects that can be applied to
tracks in the soir engine. Each track can have a set of effects with
corresponding parameters, which are typically defined using the
`tracks.setup()` function.

```python
tracks.setup({
    'bass': tracks.mk_sampler(fxs={
        'rev': fx.mk_chorus(),
    }),
})
```
"""

from dataclasses import dataclass
from typing import Any

from soir.rt._helpers import serialize_parameters
from soir.rt.ctrls import Control


@dataclass
class Fx:
    """Representation of a Soir FX.

    @public

    Attributes:
        name: The name of the effect.
        type: The effect type.
        mix: The mix parameter of the effect. Defaults to None.
        extra: Extra parameters for the effect, JSON encoded. Defaults to None.
    """

    name: str = "unnamed"
    type: str = "unknown"
    mix: float | Control | None = None
    extra: str | None = None

    def __repr__(self) -> str:
        return f"Fx(name={self.name}, type={self.type}, mix={self.mix}, extra={self.extra})"


def mk(
    type: str, mix: float | Control | None = None, extra: dict[str, Any] | None = None
) -> Fx:
    """Creates a new Fx.

    @public

    Args:
        type (str): The effect type.
        mix (float, optional): The mix parameter of the effect. Defaults to None.
        extra (dict, optional): The extra parameters of the effect. Default to None.
    """
    fx = Fx()

    fx.type = type
    fx.mix = mix
    fx.extra = serialize_parameters(extra)

    return fx


def mk_chorus(
    time: float | Control = 0.5,
    depth: float | Control = 0.5,
    rate: float | Control = 0.5,
) -> Fx:
    """Creates a new Chorus FX.

    @public

    Args:
        time: The time parameter of the chorus effect. Defaults to 0.5.
        depth: The depth parameter of the chorus effect. Defaults to 0.5.
        rate: The rate parameter of the chorus effect. Defaults to 0.5.
    """
    return mk("chorus", extra={"time": time, "depth": depth, "rate": rate})


def mk_reverb(
    mix: float | Control | None = None,
    time: float | Control = 0.01,
    wet: float | Control = 0.75,
    dry: float | Control = 0.25,
) -> Fx:
    """Creates a new Reverb FX.

    @public

    Args:
        mix: The mix parameter of the chorus effect. Defaults to None.
        time: The time parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.01.
        dry: The dry parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.25.
        wet: The wet parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.75.
    """
    return mk("reverb", mix=mix, extra={"time": time, "dry": dry, "wet": wet})


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

    @public

    The compressor can be keyed on its own input (plain in-line
    compression), on the master mix, or on the output of another track
    (sidechain compression, e.g. the kick track ducking the pads).

    External sources are tapped after the source track's own FX, before
    its fader and mute: a muted (or zero-volume) track keeps driving the
    sidechain while never being heard, which makes it a useful
    inaudible "ghost" trigger. Muting the source therefore does not
    stop the ducking; to turn the effect off live, automate `wet` to 0.

    External sources are read with a deterministic one block delay
    (~10.7 ms), which is inaudible for sidechain use.

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

    Example:
        ```python
        tracks.setup({
            'kick': tracks.mk_sampler(),
            'pads': tracks.mk_sampler(fxs={
                'duck': fx.mk_compressor(source='kick', ratio=8.0,
                                         attack=0.001, release=0.3),
            }),
        })
        ```
    """
    extra: dict[str, Any] = {
        "source": source if source is not None else "self",
        "threshold": threshold,
        "ratio": ratio,
        "attack": attack,
        "release": release,
        "knee": knee,
        "makeup": makeup,
        "wet": wet,
    }
    return mk("compressor", extra=extra)


def mk_lpf(
    mix: float | Control | None = None,
    cutoff: float | Control = 0.5,
    resonance: float | Control = 0.5,
) -> Fx:
    """Creates a new Low Pass Filter FX.

    @public

    Args:
        mix: The mix parameter of the low pass filter effect. Defaults to None.
        cutoff: The cutoff frequency of the low pass filter in the [0.0, 1.0] range. Defaults to 0.5.
        resonance: The resonance of the low pass filter in the [0.0, 1.0] range. Defaults to 0.5.
    """
    return mk("lpf", mix=mix, extra={"cutoff": cutoff, "resonance": resonance})


def mk_hpf(
    mix: float | Control | None = None,
    cutoff: float | Control = 0.5,
    resonance: float | Control = 0.5,
) -> Fx:
    """Creates a new High Pass Filter FX.

    @public

    Args:
        mix: The mix parameter of the high pass filter effect. Defaults to None.
        cutoff: The cutoff frequency of the high pass filter in the [0.0, 1.0] range. Defaults to 0.5.
        resonance: The resonance of the high pass filter in the [0.0, 1.0] range. Defaults to 0.5.
    """
    return mk("hpf", mix=mix, extra={"cutoff": cutoff, "resonance": resonance})


def mk_vst(
    plugin: str,
    mix: float | Control = 1.0,
    params: dict[str, float | Control] | None = None,
) -> Fx:
    """Creates a new VST3 effect.

    @public

    Use `vst.plugins()` to get a list of available plugins.

    Args:
        plugin: The plugin UID or name.
        mix: The dry/wet mix of the effect in the [0.0, 1.0] range: 0.0 is
             fully dry (bypasses the plugin), 1.0 is fully wet. Can be a
             Control for live automation. Defaults to 1.0.
        params: Parameter values to set. Keys are parameter names, values are
                floats in [0.0, 1.0] range or Control references.

    Example:
        ```python
        tracks.setup({
            'synth': tracks.mk_sampler(fxs={
                'eq': fx.mk_vst('FabFilter Pro-Q 3', params={'gain': 0.5}),
            }),
        })
        ```
    """
    extra: dict[str, Any] = {"plugin": plugin, "mix": mix}
    if params:
        extra["params"] = params
    return mk("vst", extra=extra)
