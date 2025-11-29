"""

The **bpm** module provides a way to set the tempo of the current
session. The tempo is a global setting that affects all loops and
samples, it is measured in beats per minute (BPM) and can be adjusted
in real-time.

# Reference
"""

from soir._bindings.rt import (
    set_bpm_,
    get_bpm_,
    get_beat_,
)
from soir.rt._internals import (
    assert_not_in_loop,
    current_loop,
)


def get() -> float:
    """Get the BPM. This function can only be called from the global
       scope.

    @public

    Returns:
        The current BPM.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    result: float = get_bpm_()
    return result


def set(bpm: float) -> float:
    """Set the BPM. This function can only be called from the global
       scope.

    @public

    Args:
        bpm: The new BPM.

    Returns:
        The new BPM.

    Raises:
        InLoopException: If called from inside a loop.
    """
    assert_not_in_loop()

    result: float = set_bpm_(bpm)
    return result


def beat() -> float:
    """Get the current beat.

    @public

    Returns:
        The current beat.
    """
    loop = current_loop()

    current_beat: float = get_beat_()
    if loop:
        return current_beat + loop.current_offset

    return current_beat
