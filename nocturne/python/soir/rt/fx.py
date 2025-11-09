"""
The `fx` module provides a set of audio effects that can be applied to
tracks in the soir engine. Each track can have a set of effects with
corresponding parameters, which are typically defined using the
`tracks.setup()` function.

# Cookbook

## Setup tracks with effects

```python
tracks.setup({
    'bass': tracks.mk_sampler(fxs={
        'rev': fx.mk_chorus(),
    }),
})
```
"""

import json

from dataclasses import dataclass
from soir.rt._helpers import serialize_parameters
from soir.rt.ctrls import Control


@dataclass
class Fx:
    """Representation of a Soir FX.

    Attributes:
        name: The name of the effect.
        type: The effect type.
        mix: The mix parameter of the effect. Defaults to None.
        extra: Extra parameters for the effect, JSON encoded. Defaults to None.
    """

    name: str = "unnamed"
    type: str = "unknown"
    mix: float | None = None
    extra: str | None = None

    def __repr__(self):
        return f"Fx(name={self.name}, type={self.type}, mix={self.mix}, extra={self.extra})"


def mk(type: str, mix: float | Control | None = None, extra: dict | None = None) -> Fx:
    """Creates a new Fx.

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

    Args:
        mix: The mix parameter of the chorus effect. Defaults to None.
        time: The time parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.01.
        dry: The dry parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.25.
        wet: The wet parameter of the reverb effect in the [0.0, 1.0] range. Defaults to 0.75.
    """
    return mk("reverb", mix=mix, extra={"time": time, "dry": dry, "wet": wet})


def mk_lpf(
    mix: float | Control | None = None,
    cutoff: float | Control = 0.5,
    resonance: float | Control = 0.5,
) -> Fx:
    """Creates a new Low Pass Filter FX.

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

    Args:
        mix: The mix parameter of the high pass filter effect. Defaults to None.
        cutoff: The cutoff frequency of the high pass filter in the [0.0, 1.0] range. Defaults to 0.5.
        resonance: The resonance of the high pass filter in the [0.0, 1.0] range. Defaults to 0.5.
    """
    return mk("hpf", mix=mix, extra={"cutoff": cutoff, "resonance": resonance})
