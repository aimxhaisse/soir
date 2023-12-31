# This code is embedded in Midi at compile time.
#
# It is executed once at startup, before accepting code from Matin
# sessions. Facilities here are for live coding, heavy code can be
# later re-written in c++ if we need to.
#
# Whatever is prefixed with __ is not meant to be used by Matin.

from enum import Enum

from _live import (
    _schedule,
    _set_bpm,
    _get_bpm,
    _get_current_beat,
    _log
)


class _LiveLoop:
    """A live loop.
    """
    def __init__(self, name: str, beats: int, align: int, func: callable):
        self.name = name
        self.beats = beats
        self.align = align
        self.func = func

    def fire(self):
        """Temporal recursion scheduling of the live loop.
        """
        at = _get_current_beat() + 1
        if self.align:
            at = self.beats - at % self.align

        def loop():
            self.func()
            _schedule(self.beats, loop)

        _schedule(at, loop)


_live_loops = dict()
_current_loop = None


def log(message: str) -> None:
    """Log a message to the console.
    """
    _log(message)

    
def set_bpm(bpm: float) -> float:
    """Set the BPM of the current session.
    """
    return _set_bpm(bpm)


def get_bpm() -> float:
    """Get the BPM of the current session.
    """
    return _get_bpm()


def sleep(beats: float):
    """Sleep for the duration of the current loop.
    """
    if not _current_loop:
        raise RuntimeError('Cannot sleep outside of a live loop.')
    current_loop.time += beats


def live_loop(*args, **kwargs):
    """Decorator to declare a live loop.

    This is actually not a standard Python decorator:

    - we want to schedule the function for execution at definition time,
    - we don't want it to be directly callable.
    """
    beats = kwargs.get('beats', 4)
    align = kwargs.get('align', beats)

    def decorator(func):
        name = func.__name__
        if name not in _live_loops:
            ll = _LiveLoop(name, beats, align, func)
            ll.fire()
            _live_loops[name] = ll
        else:
            ll = _live_loops[name]
            ll.beats = beats
            ll.align = align
            ll.func = func

        return lambda: None

    return decorator
