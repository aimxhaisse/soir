# This code is embedded in Midi at compile time.
#
# It is executed once at startup, before accepting code from Matin
# sessions. Facilities here are for live coding, heavy code can be
# later re-written in c++ if we need to.
#
# Whatever is suffixed with _ is not meant to be used by Matin.

from enum import Enum

from live_ import (
    schedule_,
    set_bpm_,
    get_bpm_,
    get_beat_,
    get_user_,
    log_
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
    def __init__(self, name: str, beats: int, align: int, func: callable):
        self.name = name
        self.beats = beats
        self.align = align
        self.func = func

        self.current_offset = 0
        self.user = get_user_()

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

# Public API

def log(message: str) -> None:
    """Log a message to the console.
    """
    global current_loop_

    if current_loop_:
        user = current_loop_.user
        schedule_(current_loop_.current_offset, lambda: log_(user, message))
    else:
        log_(get_user_(), message)


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


def live_loop(*args, **kwargs):
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
    align = kwargs.get('align', beats)

    def wrapper(func):
        """
        """
        name = func.__name__

        if name not in live_loops_:
            ll = LiveLoop_(name, beats, align, func)

            ll.run()

            live_loops_[name] = ll
        else:
            ll = live_loops_[name]

            # We allow to update the beats and align of the live loop,
            # however it will only be taken into account at the next
            # iteration of the loop.
            ll.beats = beats
            ll.align = align

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