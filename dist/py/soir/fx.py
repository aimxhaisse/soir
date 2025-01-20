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
        mix: The mix parameter of the effect. Defaults to 1.
    """
    name: str = 'Unbound'
    type: str = 'Unknown'
    mix: float | None = None

    def __repr__(self):
        return f'Fx(name={self.name}, type={self.type}, mix={self.mix})'


def mk(type: str, mix=None) -> Fx:
    """Creates a new Fx.

    Args:
        type (str): The effect type.
        mix (float, optional): The mix parameter of the effect. Defaults to None.
    """
    fx = Fx()

    fx.type = type
    fx.mix = mix

    return fx


def mk_chorus(mix=None) -> Fx:
    """Creates a new Chorus FX.

    Args:
        mix: The mix parameter of the chorus effect. Defaults to None.
    """
    return mk('chorus', mix=mix)
