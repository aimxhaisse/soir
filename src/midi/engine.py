# This code is embedded in Midi at compile time.
#
# It is executed once at startup, before accepting code from Matin
# sessions. Facilities here are for live coding, heavy code can be
# later re-written in c++ if we need to.
#
# Whatever is prefixed with __ is not meant to be used by Matin.

from enum import Enum

from __live import (
    __set_bpm,
    __get_bpm,
    __log
)


class __LiveLoop:
    """A live loop.
    """
    class State(Enum):
        """The state of a live loop.
        """
        RUNNING = 0
        SCHEDULED = 1
        STOPPED = 2
    
    def __init__(self, name: str, beats: int, align: int, func: callable):
        self.name = name
        self.beats = beats
        self.align = align
        self.func = func
        self.time = 0
        self.state = __LiveLoop.State.STOPPED

    def __call__(self, *args, **kwargs):
        """Schedules the live loop if it is not running.
        """
        if self.state == __LiveLoop.State.STOPPED:
            self.state = __LiveLoop.State.SCHEDULED
            self.func(*args, **kwargs)



__live_loops = dict()
__current_loop = None


def log(message: str) -> None:
    """Log a message to the console.
    """
    __log(message)

    
def set_bpm(bpm: float) -> float:
    """Set the BPM of the current session.
    """
    return __set_bpm(bpm)


def get_bpm() -> float:
    """Get the BPM of the current session.
    """
    return __get_bpm()


def sleep(beats: float):
    """Sleep for the duration of the current loop.
    """
    if not __current_loop:
        raise RuntimeError('Cannot sleep outside of a live loop.')
    current_loop.time += beats


def live_loop(*args, **kwargs):
    """Decorator to declare a live loop.
    """
    beats = kwargs.get('beats', 4)
    align = kwargs.get('align', beats)

    def decorator(f):
        """Decorator to declare a live loop.
        """
        def wrapper(*args, **kwargs):
            """Actual wrapper that handles rescheduling.
            """
            name = f.__name__

            if name not in __live_loops__:
                __live_loops__[name] = __LiveLoop(name, beats, align, f)
            
            return f(*args, **kwargs)

    return decorator
