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

from dataclasses import dataclass, asdict


@dataclass
class Fx:
    """Representation of a Soir FX.

    Attributes:
        name: The name of the effect.
        type: The effect type.
        mix: The mix parameter of the effect. Defaults to None.
        extra: Extra parameters for the effect, JSON encoded. Defaults to None.
    """
    name: str = 'unnamed'
    type: str = 'unknown'
    mix: float | None = None
    extra: str | None = None

    def __repr__(self):
        return f'Fx(name={self.name}, type={self.type}, mix={self.mix}, extra={self.extra})'


def mk(type: str, mix=None, extra=None) -> Fx:
    """Creates a new Fx.

    Args:
        type (str): The effect type.
        mix (float, optional): The mix parameter of the effect. Defaults to None.
        extra (dict, optional): The extra parameters of the effect. Default to None.
    """
    fx = Fx()

    fx.type = type
    fx.mix = mix
    fx.extra = json.dumps(extra)

    return fx


def mk_chorus(mix=None) -> Fx:
    """Creates a new Chorus FX.

    Args:
        mix: The mix parameter of the chorus effect. Defaults to None.
    """
    return mk('chorus', mix=mix, extra=None)


def mk_reverb(mix=None) -> Fx:
    """Creates a new Reverb FX.

    Args:
        mix: The mix parameter of the chorus effect. Defaults to None.
    """
    return mk('reverb', mix=mix, extra=None)
