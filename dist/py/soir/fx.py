"""
The `fx` module provides a set of audio effects that can be applied to
tracks in the soir engine. Each track can have a set of effects with
corresponding parameters, which are typically defined using the
`tracks.setup()` function.

# Cookbook

## Setup tracks with effects


```python
tracks.setup({
    'bass': tracks.mk_sampler(fx={
        'rev': fx.mk_reverb(mix=0.2),
    }),
    'melody': tracks.mk_sampler()
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
        mix: The mix parameter of the effect. Defaults to 1.
    """
    name: str | Optional = None
    type: str = 'Unknown'
    mix: float = 1.0

    def __repr__(self):
        return f'Fx(name={self.name}, type={self.type}, mix={self.mix})'


def mk(type: str, mix: float = 1.0, name: str | Optional) -> Fx:
    """Creates a new Fx.

    Args:
        type: The effect type.
        mix: The mix parameter of the effect. Defaults to 1.
    """
    fx = Fx()

    fx.type = type
    fx.mix = mix
    fx.name = name

    return fx


def mk_chorus(mix: float = 1.0) -> Fx:
    """Creates a new Chorus FX.

    Args:
        mix: The mix parameter of the chorus effect. Defaults to 1.
    """
    return mk('chorus', mix=mix)
