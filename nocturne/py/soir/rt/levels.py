"""
Level metering for tracks and master output.

@public

The **levels** module provides access to audio level metering for both
individual tracks and the master output. Levels include peak amplitude
(with hold/decay) and RMS values for left and right channels.

```python
# Get master output levels
master = levels.get_master_levels()
print(f"Master peak: L={master.peak_left:.3f} R={master.peak_right:.3f}")

# Get levels for a specific track
drums = levels.get_track_level("drums")
print(f"Drums peak: L={drums.peak_left:.3f} R={drums.peak_right:.3f}")
```
"""

from dataclasses import dataclass

from soir._bindings.rt import get_master_levels_, get_track_levels_, get_track_level_
from soir.rt.errors import TrackNotFoundException


@dataclass
class Levels:
    """Audio levels for a track or master output.

    @public

    Attributes:
        peak_left: Peak amplitude for the left channel (0.0 to 1.0+).
        peak_right: Peak amplitude for the right channel (0.0 to 1.0+).
        rms_left: RMS level for the left channel.
        rms_right: RMS level for the right channel.
    """

    peak_left: float
    peak_right: float
    rms_left: float
    rms_right: float


def get_track_levels() -> dict[str, Levels]:
    """Get current levels for all tracks.

    @public

    Returns:
        Dictionary mapping track names to their current levels.
    """
    raw = get_track_levels_()
    return {name: Levels(**data) for name, data in raw.items()}


def get_track_level(name: str) -> Levels:
    """Get current levels for a specific track.

    @public

    Args:
        name: The track name.

    Returns:
        The track's current levels.

    Raises:
        TrackNotFoundException: If the track doesn't exist.
    """
    raw = get_track_level_(name)
    if raw is None:
        raise TrackNotFoundException(f"Track '{name}' not found")
    return Levels(**raw)


def get_master_levels() -> Levels:
    """Get current master output levels.

    @public

    Returns:
        The master output levels.
    """
    return Levels(**get_master_levels_())
