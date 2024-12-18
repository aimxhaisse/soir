"""
The `tracks` module provides a way to setup and control tracks in the
soir engine. A track has an instrument type and a set of parameters
and effects. Once a track is created, loops can be scheduled on
it. Tracks can be added & removed in real-time using the `setup()`
function, existing tracks are untouched.

# Cookbook

## Setup tracks

``` python
tracks.setup([
    tracks.mk('sampler', 0),
    tracks.mk('sampler', 1),
    tracks.mk('sampler', 2),
])
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
from soir.internals import (
    assert_not_in_loop,
)


@dataclass
class Track:
    """Representation of a Soir track.

    Attributes:
        instrument: The instrument type.
        track: The track id.
        muted: The muted state. Defaults to None.
        volume: The volume. Defaults to None.
        pan: The pan. Defaults to None.
        extra: Extra parameters. Defaults to None.
    """
    instrument: str = None
    track: int = None
    muted: bool | None = None
    volume: float | None  = None
    pan: float | None = None
    extra: dict | None = None

    def __repr__(self):
        return f'Track(instrument={self.instrument}, track={self.track}, muted={self.muted}, volume={self.volume}, pan={self.pan})'


def layout() -> list[Track]:
    """Get the current tracks.

    Returns:
        list[Track]: The current tracks.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    return [Track(**t) for t in get_tracks_()]


def setup(tracks: list[Track]) -> bool:
    """Setup tracks.

    Args:
        tracks (list[Track]): The tracks to setup.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    return setup_tracks_([asdict(track) for track in tracks])


def mk(instrument: str, track: int, muted=None, volume=None, pan=None, extra=None) -> Track:
    """Creates a new track.

    Args:
        instrument (str): The instrument type.
        track (int): The track number.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float, optional): The volume. Defaults to None.
        pan (float, optional): The pan. Defaults to None.
        extra (dict, optional): Extra parameters. Defaults to None.
    """
    t = Track()

    t.instrument = instrument
    t.track = track
    t.muted = muted
    t.volume = volume
    t.pan = pan
    t.extra = json.dumps(extra)

    return t


def mk_sampler(track: int, muted=None, volume=None, pan=None, extra=None) -> Track:
    """Creates a new sampler track.

    Args:
        track (int): The track id.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float, optional): The volume. Defaults to None.
        pan (float, optional): The pan. Defaults to None.
        extra (dict, optional): Extra parameters. Defaults to None.
    """
    return mk('sampler', track, muted, volume, pan, extra)


def mk_midi(track: int, muted=None, volume=None, pan=None, midi_device=0, audio_device=0) -> Track:
    """Creates a new midi track.

    Args:
        track (int): The track id.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float, optional): The volume. Defaults to None.
        pan (float, optional): The pan. Defaults to None.
        midi_device (int, optional): The midi device. Defaults to -1.
        audio_device (int, optional): The audio device. Defaults to -1.
    """
    return mk('midi_ext', track, muted, volume, pan, extra={'midi_device': midi_device, 'audio_device': audio_device})
