"""*Track them all.*

The `tracks` module provides a way to setup and control tracks in the
neon engine. A track has an instrument type and a set of parameters
and effects. Once a track is created, loops can be scheduled on
it. Tracks can be added & removed in real-time using the `setup()`
function, existing tracks are untouched.

# Cookbook

## Setup tracks

``` python
tracks.setup([
    tracks.mk('mono_sampler', 0),
    tracks.mk('mono_sampler', 1),
    tracks.mk('mono_sampler', 2),
])
```

## Get current tracks

``` python
trks = tracks.layout()
```

# Reference
"""

from dataclasses import dataclass, asdict

from bindings import (
    get_tracks_,
    setup_tracks_,
)
from neon.errors import (
    InLiveLoopException,
)
from neon.internals import (
    assert_not_in_loop,
)


@dataclass
class Track:
    """Representation of a Neon track.

    Attributes:
        instrument: The instrument type.
        channel: The channel.
        muted: The muted state. Defaults to None.
        volume: The volume. Defaults to None.
        pan: The pan. Defaults to None.
        extra: Extra parameters. Defaults to None.
    """
    instrument: str = None
    channel: int = None
    muted: bool | None = None
    volume: float | None  = None
    pan: float | None = None
    extra: dict | None = None

    def __repr__(self):
        return f'Track(instrument={self.instrument}, channel={self.channel}, muted={self.muted}, volume={self.volume}, pan={self.pan})'


def layout() -> list[Track]:
    """Get the current tracks.

    Returns:
        list[Track]: The current tracks.

    Raises:
        InLiveLoopException: If called from inside a live loop.
    """
    assert_not_in_loop()

    return [Track(**t) for t in get_tracks_()]


def setup(tracks: list[Track]) -> bool:
    """Setup tracks.

    Args:
        tracks (list[Track]): The tracks to setup.

    Raises:
        InLiveLoopException: If called from inside a live loop.
    """
    assert_not_in_loop()

    return setup_tracks_([asdict(track) for track in tracks])


def mk(instrument: str, channel: int, muted=None, volume=None, pan=None) -> Track:
    """Creates a new track.

    Args:
        instrument (str): The instrument type.
        channel (int): The channel.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float, optional): The volume. Defaults to None.
        pan (float, optional): The pan. Defaults to None.
    """
    track = Track()

    track.instrument = instrument
    track.channel = channel
    track.muted = muted
    track.volume = volume
    track.pan = pan

    return track

