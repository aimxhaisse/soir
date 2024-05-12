"""*Push the tempo.*

The **bpm** module provides a way to set the tempo of the current
session. The tempo is a global setting that affects all loops and
samples. The tempo is measured in beats per minute (BPM) and can be
adjusted in real-time.

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
beat = bpm.get_beat()
```

# Reference
"""

from live_ import (
    set_bpm_,
    get_bpm_,
    get_beat_,
)

def get_bpm() -> float:
    """Get the BPM.

    Returns:
        The current BPM.
    """
    global current_loop_

    if current_loop_:
        raise InLiveLoopException()

    return get_bpm_()


def set_bpm(bpm: float) -> float:
    """Set the BPM.

    Args:
        bpm: The new BPM.

    Returns:
        The new BPM.
    """
    global current_loop_

    if current_loop_:
        raise InLiveLoopException()

    return set_bpm_(bpm)


def get_beat() -> float:
    """Get the current beat.

    Returns:
        The current beat.
    """
    global current_loop_

    if current_loop_:
        return get_beat_() + current_loop_.current_offset

    return get_beat_()
