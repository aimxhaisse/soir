# This code is embedded in Midi at compile time.
#
# It is executed once at startup, before accepting code from Matin
# sessions. Facilities here are for live coding, heavy code can be
# later re-written in C++ if we need to.

import json

from enum import Enum

from live_ import (
    Track,
    schedule_,
    set_bpm_,
    get_bpm_,
    get_beat_,
    get_tracks_,
    log_,
    midi_note_on_,
    midi_note_off_,
    midi_cc_,
    setup_tracks_,
)


class LiveException(Exception):
    """Base class for exceptions in this module.
    """
    pass


class NotInLiveLoopException(LiveException):
    """Raised when trying to execute code outside of a live loop.
    """
    pass


class InLiveLoopException(LiveException):
    """Raised when trying to execute code inside a live loop.
    """
    pass


live_loops_ = dict()
current_loop_ = None


class LiveLoop_:
    """Helper class to manage a live loop.
    """
    def __init__(self, name: str, beats: int, track: int, align: int, func: callable):
        self.name = name
        self.beats = beats
        self.track = track
        self.align = align
        self.func = func

        self.current_offset = 0

    def reset(self):
        """Reset the live loop.
        """
        self.current_offset = 0

    def run(self):
        """Temporal recursion scheduling of the live loop.
        """
        at = get_beat_()
        if self.align:
            at = self.beats - at % self.align

        def loop():
            global current_loop_

            self.reset()

            current_loop_ = self
            self.func()
            current_loop_ = None

            schedule_(self.beats, loop)

        schedule_(at, loop)


def loop(*args, **kwargs):
    """A live loop.

    The concept of live loop is similar to Sonic Pi's live loops. Code
    within a live loop is executed in a temporal recursion, and can
    updated in real-time. Few rules need to be followed form code in
    live-loops: it should not be blocking as it would block the main
    thread. For this reason, blocking facilities such are sleep are
    handled by the engine.
    """
    # Parameters of the decorator.
    beats = kwargs.get('beats', 4)
    track = kwargs.get('track', 1)
    align = kwargs.get('align', beats)

    def wrapper(func):
        """
        """
        name = func.__name__

        if name not in live_loops_:
            ll = LiveLoop_(name, beats, track, align, func)

            ll.run()

            live_loops_[name] = ll
        else:
            ll = live_loops_[name]

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


# Utilities API

def log(message: str) -> None:
    """Log a message to the console.
    """
    global current_loop_

    if current_loop_:
        schedule_(current_loop_.current_offset, lambda: log_(message))
    else:
        log_(message)


def get_beat() -> float:
    """Get the current beat.
    """
    global current_loop_

    if current_loop_:
        return get_beat_() + current_loop_.current_offset

    return get_beat_()


def sleep(beats: float):
    """Sleep for the duration of the current loop.
    """
    global current_loop_

    if not current_loop_:
        raise NotInLiveLoopException()

    current_loop_.current_offset += beats


# BPM API

def set_bpm(bpm: float) -> float:
    """Set the BPM.
    """
    global current_loop_

    if current_loop_:
        raise InLiveLoopException()

    return set_bpm_(bpm)


def get_bpm() -> float:
    """Get the BPM.
    """
    global current_loop_

    if current_loop_:
        raise InLiveLoopException()

    return get_bpm_()


# Tracks API


def get_tracks() -> list[Track]:
    """Get the tracks.
    """
    global current_loop_
 
    if current_loop_:
        raise InLiveLoopException()

    return get_tracks_()


def setup_tracks(tracks: list[Track]) -> bool:
    """Setup tracks.
    """
    global current_loop_

    if current_loop_:
        raise InLiveLoopException()

    return setup_tracks_(tracks)


def mk_track(instrument: str, channel: int, muted=None, volume=None, pan=None) -> Track:
    """Creates a new track.
    """
    track = Track()

    track.instrument = instrument
    track.channel = channel
    track.muted = muted
    track.volume = volume
    track.pan = pan

    return track


# Midi API


def midi_note_on(channel: int, note: int, velocity: int):
    """Send a MIDI note on message.
    """
    global current_loop_

    if current_loop_:
        schedule_(current_loop_.current_offset, lambda: midi_note_on_(channel, note, velocity))
    else:
        midi_note_on_(channel, note, velocity)


def midi_note_off(channel: int, note: int, velocity: int):
    """Send a MIDI note off message.
    """
    global current_loop_

    if current_loop_:
        schedule_(current_loop_.current_offset, lambda: midi_note_off_(channel, note, velocity))
    else:
        midi_note_off_(channel, note, velocity)


def midi_cc(channel: int, cc: int, value: int):
    """Send a MIDI CC message.
    """
    global current_loop_

    if current_loop_:
        schedule_(current_loop_.current_offset, lambda: midi_cc_(channel, cc, value))
    else:
        midi_cc_(channel, cc, value)
