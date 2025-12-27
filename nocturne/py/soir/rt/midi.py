"""
The **midi** module provides a way to communicate with external
synthesizers.

@public

``` python
with midi.use_chan(1):
  midi.note_on(60)
  sleep(1)
  midi.note_off(60)
```

"""

from soir._bindings.rt import (
    midi_note_on_,
    midi_note_off_,
    schedule_,
)
from soir.rt._internals import (
    assert_in_loop,
)
from soir.rt.errors import (
    UnknownMidiTrackException,
)


class use_chan:
    """Context manager to set the MIDI channel to use.

    @public

    `use_chan` can be used to send MIDI events to a specific channel. It
    can be used as a context manager to set the MIDI channel to use, or
    as a function from within a loop.

    Examples:

    ```python
    @loop(track='bass', beats=4)
    def my_loop:
        # This block will send MIDI events to channel 1 on the 'bass' track.
        with midi.use_chan(1):
            midi.note_on(60)
            sleep(1)
            midi.note_off(60)

       # From this point on, MIDI events will be sent to channel 3 on the 'bass' track.
       midi.use_chan(3)
       midi.note_on(60)
    ```
    """

    def __init__(self, chan: int):
        self.loop = assert_in_loop()
        self.previous_chan: int | str | None = self.loop.extra.get("midi_chan")
        self.loop.extra["midi_chan"] = chan

    def __enter__(self) -> None:
        pass

    def __exit__(self, exc_type: object, exc_value: object, traceback: object) -> None:
        if self.previous_chan is not None:
            self.loop.extra["midi_chan"] = self.previous_chan


def _get_chan(chan: int | None = None) -> int:
    """Internal helper to get the MIDI chan to use.

    Args:
        chan: The MIDI chan to use if provided by user.

    Returns:
        The MIDI chan to use.
    """
    loop = assert_in_loop()

    if chan is not None:
        return chan

    midi_chan = loop.extra.get("midi_chan")
    if midi_chan:
        return int(midi_chan)

    raise UnknownMidiTrackException()


def note_on(note: int, velocity: int = 127, chan: int | None = None) -> None:
    """Send the MIDI note to the external synthesizer configured on the track.

    @public
    
    Args:
        note: The MIDI note to send.
        velocity: The velocity. Defaults to 127.
        chan: The MIDI chan to send the note to. Uses the value from use_chan() if not provided.

    Raises:
        NotInLoopException: If called from outside a loop.
    """
    loop = assert_in_loop()
    chan = _get_chan(chan)

    track = loop.track
    if not track:
        raise UnknownMidiTrackException()

    schedule_(loop.current_offset, lambda: midi_note_on_(track, chan, note, velocity))


def note_off(note: int, velocity: int = 127, chan: int | None = None) -> None:
    """Send the MIDI note off to the external synthesizer using the track id of the loop as MIDI channel.

    @public

    Args:
        note: The MIDI note to stop.
        velocity: The velocity. Defaults to 127.
        chan: The MIDI chan to stop the note on. Uses the value from use_chan() if not provided.

    Raises:
        NotInLoopException: If called from outside a loop.
    """
    loop = assert_in_loop()
    chan = _get_chan(chan)

    track = loop.track
    if not track:
        raise UnknownMidiTrackException()

    schedule_(loop.current_offset, lambda: midi_note_off_(track, chan, note, velocity))


def note(
    note: int, duration: float, velocity: int = 127, chan: int | None = None
) -> None:
    """Send the MIDI note on and off to the external synthesizer using the track id of the loop as MIDI channel.

    @public

    Args:
        note: The MIDI note to send.
        duration: The duration of the note in beats.
        velocity: The velocity. Defaults to 127.
        chan: The MIDI chan to send the note to.

    Raises:
        NotInLoopException: If called from outside a loop.
    """
    loop = assert_in_loop()
    chan = _get_chan(chan)

    track = loop.track
    if not track:
        raise UnknownMidiTrackException()

    schedule_(loop.current_offset, lambda: midi_note_on_(track, chan, note, velocity))

    schedule_(
        loop.current_offset + duration,
        lambda: midi_note_off_(track, chan, note, velocity),
    )
