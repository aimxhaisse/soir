import json

from enum import Enum

from bindings import (
    schedule_,
    get_beat_,
    get_tracks_,
    log_,
    midi_note_on_,
    midi_note_off_,
    midi_cc_,
    setup_tracks_,
)
from neon.errors import (
    InLiveException,
    NotInLiveException,
)


# Registry of all known decorated functions (@live, @loop).
live_registry_ = dict()

# Currently executing decorated function.
current_live_ = None


class Live_:
    """Helper class to manage a live function.
    """
    def __init__(self, name: str, beats: int, track: int, align: int, loop: bool, func: callable):
        self.name = name
        self.beats = beats
        self.track = track
        self.align = align
        self.func = func
        self.loop = loop

        self.current_offset = 0

    def reset(self):
        """Reset the live loop.
        """
        self.current_offset = 0

    def run(self):
        """Temporal recursion scheduling of the live loop.

        If the live is not looping, only schedule once.
        """
        at = get_beat_()

        if self.align:
            at = self.beats - at % self.align

        def loop():
            global current_live_

            self.reset()

            current_live_ = self
            self.func()
            current_live_ = None

            if self.loop:
                schedule_(self.beats, loop)

        schedule_(at, loop)


def _live(beats: int, track: int, align: int, loop: bool) -> callable:

    def wrapper(func):
        """
        """
        name = func.__name__

        if name not in live_registry_:
            ll = Live_(name, beats, track, align, loop, func)

            ll.run()

            live_registry_[name] = ll
        else:
            ll = live_registry_[name]

            # We allow to update the beats and align of the live loop,
            # however it will only be taken into account at the next
            # iteration of the loop.
            ll.beats = beats
            ll.align = align
            ll.track = track

            # Here is where we perform code update seamlessly.
            ll.func = func

        # This is a bit counter-intuitive: we don't allow to execute
        # the decorated function directly, it is scheduled the moment
        # the @live_loop decorator is called at definition time. We
        # could maybe raise an exception here to help the user, but at
        # the moment we don't have an error notification flow
        # per-user.

        return lambda: None

    return wrapper


def loop(beats: int, track: int = 0, align: int = 0) -> callable:
    """Decorator to define a live loop.

    Args:
        beats: The duration of the loop in beats.
        track: The track to use for the loop.
        align: The alignment of the loop.

    Returns:
        A callable that can be used to decorate a function.
    """
    return _live(beats, track, align, loop=True)


def live(beats: int, track: int = 0, align: int = 0) -> callable:
    """Decorator to define a live function.

    Args:
        beats: The duration of the loop in beats.
        track: The track to use for the loop.
        align: The alignment of the loop.

    Returns:
        A callable that can be used to decorate a function.
    """
    return _live(beats, track, align, loop=False)


def assert_in_live():
    """Assert that we are in a live context.

    Raises:
        NotInLiveException: If we are not in a live loop.
    """
    global current_live_

    if not current_live_:
        raise NotInLiveException()


def assert_not_in_live():
    """Assert that we are not in a live context.

    Raises:
        InLiveException: If we are in a live loop.
    """
    global current_live_

    if current_live_:
        raise InLiveException()


def current_live() -> Live_:
    """Get the current live context.

    Returns:
        The current live context.
    """
    global current_live_

    return current_live_    


# Utilities API


def log(message: str) -> None:
    """Log a message to the console.
    """
    global current_live_

    if current_live_:
        schedule_(current_live_.current_offset, lambda: log_(message))
    else:
        log_(message)


def sleep(beats: float):
    """Sleep for the duration of the current loop.
    """
    global current_live_

    if not current_live_:
        raise NotInLiveException()

    current_live_.current_offset += beats


# Midi API


def midi_note_on(channel: int, note: int, velocity: int):
    """Send a MIDI note on message.
    """
    global current_live_

    if current_live_:
        schedule_(current_live_.current_offset, lambda: midi_note_on_(channel, note, velocity))
    else:
        midi_note_on_(channel, note, velocity)


def midi_note_off(channel: int, note: int, velocity: int):
    """Send a MIDI note off message.
    """
    global current_live_

    if current_live_:
        schedule_(current_live_.current_offset, lambda: midi_note_off_(channel, note, velocity))
    else:
        midi_note_off_(channel, note, velocity)


def midi_cc(channel: int, cc: int, value: int):
    """Send a MIDI CC message.
    """
    global current_live_

    if current_live_:
        schedule_(current_live_.current_offset, lambda: midi_cc_(channel, cc, value))
    else:
        midi_cc_(channel, cc, value)
