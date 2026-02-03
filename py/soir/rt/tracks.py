"""
The `tracks` module provides a way to setup and control tracks in the
soir engine. A track has an instrument type and a set of parameters
and effects. Once a track is created, loops can be scheduled on
it. Tracks can be added & removed in real-time using the `setup()`
function, existing tracks are untouched.

@public

```python
tracks.setup({
    'bass': tracks.mk_sampler(fxs={
        'reverb': fx.mk_reverb(mix=0.2),
    }),
    'melody': tracks.mk_sampler()
})
```
"""

import json
from typing import Any

from dataclasses import (
    dataclass,
    asdict,
    field,
)
from soir._bindings.rt import (
    get_tracks_,
    setup_tracks_,
)
from soir.rt.ctrls import (
    Control,
)
from soir.rt._ctrls import (
    controls_registry_,
)
from soir.rt._internals import (
    assert_not_in_loop,
)
from soir.rt.fx import Fx


@dataclass
class Track:
    """Representation of a Soir track.

    @public

    Attributes:
        name: The track name.
        instrument: The instrument type.
        muted: The muted state. Defaults to None.
        volume: The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan: The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        fxs: The effects as an ordered dict mapping names to Fx objects.
        extra: Extra parameters, JSON encoded. Defaults to None.
    """

    name: str = "unnamed"
    instrument: str = "unknown"

    muted: bool | None = None
    volume: float | Control = 1.0
    pan: float | Control = 0.0
    fxs: dict[str, Fx] = field(default_factory=dict)
    extra: str | None = None

    def __repr__(self) -> str:
        fxs_types = [fx.type for fx in self.fxs.values()]
        return f"Track(name={self.name}, instrument={self.instrument}, muted={self.muted}, volume={self.volume}, pan={self.pan}, fxs={fxs_types})"


def layout() -> dict[str, Track]:
    """Get the current tracks.

    @public

    Returns:
        dict[Track]: The current tracks.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    tracks: dict[str, Track] = {}
    for trk in get_tracks_():
        # Here we translate back control names into actual Control
        # parameters, this allows setup()/layout() to be somewhat
        # idempotent calls using the same parameter formats.
        params: dict[str, object] = {}
        for k, v in trk.items():
            if k == "fxs":
                # Convert list of dicts from C++ to dict[str, Fx]
                fxs_dict: dict[str, Fx] = {}
                for fx_dict in v:
                    fxs_dict[fx_dict["name"]] = Fx(
                        name=fx_dict["name"],
                        type=fx_dict["type"],
                        extra=fx_dict.get("extra"),
                    )
                params[k] = fxs_dict
            elif isinstance(v, str) and k not in ["name", "instrument"]:
                # Translate control names back to Control objects
                params[k] = controls_registry_[v]
            else:
                params[k] = v
        tracks[str(params["name"])] = Track(**params)  # type: ignore[arg-type]

    return tracks


def setup(tracks: dict[str, Track]) -> bool:
    """Setup tracks.

    @public

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
        if track.fxs:
            for fx_name, fx in track.fxs.items():
                fx.name = fx_name
                fxs.append(asdict(fx))
        track_dict[name] = asdict(track)
        track_dict[name]["fxs"] = fxs

    return setup_tracks_(track_dict)  # type: ignore[no-any-return]


def mk(
    instrument: str,
    muted: bool | None = None,
    volume: float | Control = 1.0,
    pan: float | Control = 0.0,
    fxs: dict[str, Fx] | None = None,
    extra: dict[str, object] | None = None,
) -> Track:
    """Creates a new track.

    @public

    Args:
        instrument (str): The instrument type.
        muted (bool, optional): The muted state. Defaults to None.
        volume (float | Control): The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan (float | Control): The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        fxs: The effects to apply to the track, as an ordered dict.
        extra (dict, optional): Extra parameters. Defaults to None.
    """
    t = Track()
    t.instrument = instrument
    t.muted = muted
    t.volume = volume
    t.pan = pan
    t.fxs = fxs if fxs is not None else {}
    t.extra = json.dumps(extra)

    return t


def mk_sampler(
    muted: bool | None = None,
    volume: float | Control = 1.0,
    pan: float | Control = 0.0,
    fxs: dict[str, Fx] | None = None,
) -> Track:
    """Creates a new sampler track.

    @public

    Args:
        muted (bool, optional): The muted state. Defaults to None.
        volume (float | Control): The volume in the [0.0, 1.0] range. Defaults to 1.0.
        pan (float | Control): The pan in the [-1.0, 1.0] range. Defaults to 0.0.
        fxs: The effects to apply to the track, as an ordered dict.
    """
    return mk("sampler", muted, volume, pan, fxs, extra={})


def mk_external(
    muted: bool | None = None,
    volume: float | Control = 1.0,
    pan: float | Control = 0.0,
    midi_out: str | None = None,
    audio_in: str | None = None,
    audio_chans: list[int] | None = None,
    fxs: dict[str, Fx] | None = None,
) -> Track:
    """Creates a new external device track.

    @public

    Args:
        muted: The muted state.
        volume: The volume in [0.0, 1.0].
        pan: The pan in [-1.0, 1.0].
        midi_out: MIDI output device name.
        audio_in: Audio input device name.
        audio_chans: Channel mapping [L_source, R_source]. Required if audio_in set.
        fxs: The effects to apply to the track, as an ordered dict.
    """
    if midi_out is None and audio_in is None:
        raise ValueError("At least one of midi_out or audio_in must be specified")

    extra: dict[str, Any] = {}

    if midi_out is not None:
        extra["midi_out"] = midi_out

    if audio_in is not None:
        if audio_chans is None or len(audio_chans) != 2:
            raise ValueError("audio_chans must have exactly 2 elements [L, R]")
        extra["audio_in"] = audio_in
        extra["audio_channels"] = audio_chans

    return mk("external", muted, volume, pan, fxs, extra=extra)
