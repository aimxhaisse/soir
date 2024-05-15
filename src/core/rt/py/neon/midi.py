"""*Insert a joke here.*

The **midi** module provides a way to communicate with external
synthesizers.

# Cookbook

``` python
midi.note_on(60)
midi.note_off(60)
```

# Reference
"""

from bindings import (
    midi_note_on_,
    midi_note_off_,
)
from neon.errors import (
    NotInLiveLoopException,
)
from neon.internals import (
    assert_in_loop,
    current_loop_,
)


def note_on(note: int, velocity: int = 127) -> float:
    """Send the MIDI note to the external synthesizer.

    Args:
        note: The MIDI note to send.
        velocity: The velocity. Defaults to 127.

    Raises:
        NotInLiveLoopException: If called from outside a live loop.
    """
    assert_in_loop()

    track = current_loop().track

    schedule_(
        current_loop().current_offset,
        lambda: midi_note_on_(track.channel, note, velocity)
    )

    
def note_off(note: int, velocity: int = 127) -> float:
    """Send the MIDI note off to the external synthesizer.

    Args:
        note: The MIDI note to stop.
        velocity: The velocity. Defaults to 127.

    Raises:
        NotInLiveLoopException: If called from outside a live loop.
    """
    assert_in_loop()

    track = current_loop().track

    schedule_(
        current_loop().current_offset,
        lambda: midi_note_off_(track.channel, note, velocity)
    )
