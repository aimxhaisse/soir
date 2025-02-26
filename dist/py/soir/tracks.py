"""
The `tracks` module provides a way to setup and control tracks in the
soir engine. A track has an instrument type and a set of parameters
and effects. Once a track is created, loops can be scheduled on
it. Tracks can be added & removed in real-time using the `setup()`
function, existing tracks are untouched.

# Cookbook

## Setup tracks

```python
tracks.setup({
    'bass': tracks.mk_sampler(fxs={
        'reverb': fx.mk_reverb(mix=0.2),
    }),
    'melody': tracks.mk_sampler()
})
```

## Get current tracks

``` python
trks = tracks.layout()
```

# Reference
"""

import json

from dataclasses import dataclass, asdict

from bindings import (
    get_tracks_,
    setup_tracks_,
)
from soir.errors import (
    InLoopException,
)
from soir.fx import (
    Fx,
)
from soir.ctrls import (
    Control,
)
from soir._internals import (
    assert_not_in_loop,
)


@dataclass
class Track:
    """Representation of a Soir track.

    Attributes:
        name: The track name.
        instrument: The instrument type.
        muted: The muted state. Defaults to None.
        volume: The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan: The pan in the [-1.0, 0.0] range. Defaults to 0.0.
        fxs: The effects. Defaults to None.
        extra: Extra parameters, JSON encoded. Defaults to None.
    """
    name: str = 'unnamed'
    instrument: str = 'unknown'
    muted: bool | None = None
    volume: float | Control = 1.0
    pan: float | Control = 0.0
    fxs: dict | None = None
    extra: str | None = None

    def __repr__(self):
        return f'Track(name={self.name}, instrument={self.instrument}, muted={self.muted}, volume={self.volume}, pan={self.pan}, fxs={self.fxs})'


def layout() -> dict[Track]:
    """Get the current tracks.

    Returns:
        dict[Track]: The current tracks.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    tracks = {}
    for trk in get_tracks_():
        print(str(trk))
        tracks['name']: Track(**trk)

    return tracks


def setup(tracks: dict[str, Track]) -> bool:
    """Setup tracks.

    Args:
        tracks (dict[str, Track]): The tracks to setup.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    track_dict = {}
    for name, track in tracks.items():
        # Setup names of tracks and FX based on the key of the dict
        # that defines them. This is done at the setup stage to avoid
        # double-repeat the effect name.
        track.name = name
        fxs = []
        print(track)
        if track.fxs:
            for fx_name, fx in track.fxs.items():
                fx.name = fx_name
                fxs.append(asdict(fx))
        track_dict[name] = asdict(track)
        track_dict[name]['fxs'] = fxs

    return setup_tracks_(track_dict)


def mk(instrument: str, muted=None, volume=1.0, pan=0.0, fxs=None, extra=None) -> Track:
    """Creates a new track.

    Args:
        instrument (str): The instrument type.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float | Control): The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan (float | Control): The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        fxs (dict, optional): The effects to apply to the track. Defaults to None.
        extra (dict, optional): Extra parameters. Defaults to None.
    """
    t = Track()

    t.instrument = instrument
    t.muted = muted
    t.volume = volume
    t.pan = pan
    t.fxs = fxs
    t.extra = json.dumps(extra)

    return t


def mk_sampler(muted=None, volume=1.0, pan=0.0, extra=None) -> Track:
    """Creates a new sampler track.

    Args:
        muted (bool, optional): The muted state. Defaults to None.
        volume (float | Control): The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan (float | Control): The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        extra (dict, optional): Extra parameters. Defaults to None.
    """
    return mk('sampler', muted, volume, pan, extra)


def mk_midi(muted=None, volume=1.0, pan=0.0, midi_device=0, audio_device=0) -> Track:
    """Creates a new midi track.

    Args:
        track (int): The track id.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float | Control): The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan (float | Control): The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        midi_device (int, optional): The midi device. Defaults to -1.
        audio_device (int, optional): The audio device. Defaults to -1.
    """
    return mk('midi_ext', muted, volume, pan, extra={'midi_device': midi_device, 'audio_device': audio_device})
