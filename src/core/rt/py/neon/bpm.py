"""*Push the tempo.*

The **bpm** module provides a way to set the tempo of the current
session. The tempo is a global setting that affects all loops and
samples, it is measured in beats per minute (BPM) and can be adjusted
in real-time.

# Cookbook

## Set the tempo

``` python
bpm.set(120)
```

## Get the current tempo

``` python
tempo = bpm.get()
```

## Get the current beat

``` python
beat = bpm.beat()
```

# Reference
"""

from neon.errors import (
    InLiveLoopException,
)

from neon.internals import (
    current_loop_,
)

from live_ import (
    set_bpm_,
    get_bpm_,
    get_beat_,
)


def get() -> float:
    """Get the BPM. This function can only be called from the global
       scope.

    Returns:
        The current BPM.

    Raises:
        InLiveLoopException: If called from inside a live loop.
    """
    if current_loop_:
        raise InLiveLoopException()

    return get_bpm_()


def set(bpm: float) -> float:
    """Set the BPM. This function can only be called from the global
       scope.

    Args:
        bpm: The new BPM.

    Returns:
        The new BPM.

    Raises:
        InLiveLoopException: If called from inside a live loop.
    """
    if current_loop_:
        raise InLiveLoopException()

    return set_bpm_(bpm)


def beat() -> float:
    """Get the current beat.

    Returns:
        The current beat.
    """
    if current_loop_:
        return get_beat_() + current_loop_.current_offset

    return get_beat_()
