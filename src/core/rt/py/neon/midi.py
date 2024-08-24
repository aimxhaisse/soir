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
    schedule_,
)
from neon.internals import (
    assert_in_loop,
    current_loop,
)


def note_on(note: int, velocity: int = 127) -> float:
    """Send the MIDI note to the external synthesizer using the track id of the loop as MIDI channel.

    Args:
        note: The MIDI note to send.
        velocity: The velocity. Defaults to 127.

    Raises:
        NotInLoopException: If called from outside a loop.
    """
    loop = assert_in_loop()

    track = loop.track

    schedule_(
        loop.current_offset,
        lambda: midi_note_on_(track, note, velocity)
    )

    
def note_off(note: int, velocity: int = 127) -> float:
    """Send the MIDI note off to the external synthesizer using the track id of the loop as MIDI channel.

    Args:
        note: The MIDI note to stop.
        velocity: The velocity. Defaults to 127.

    Raises:
        NotInLoopException: If called from outside a loop.
    """
    loop = assert_in_loop()

    track = loop.track

    schedule_(
        loop.current_offset,
        lambda: midi_note_off_(track, note, velocity)
    )
