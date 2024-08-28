import json
import inspect

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
    InLoopException,
    NotInLoopException,
)


# Decorator API for @loop


loop_registry_ = dict()
current_loop_ = None


class Loop_:
    """Helper class to manage a loop function.
    """
    def __init__(self, name: str, beats: int, track: int, align: int, func: callable):
        self.name = name
        self.beats = beats
        self.track = track
        self.align = align
        self.func = func

        # Current offset in beats in the loop, used to schedule events
        # in the future.
        self.current_offset = 0

    def run(self):
        """Temporal recursion scheduling of the loop.
        """
        at = get_beat_()

        if self.align:
            at = self.beats - at % self.align

        def loop():
            global current_loop_

            current_loop_ = self
            self.current_offset = 0
            self.func()
            current_loop_ = None

            schedule_(self.beats, loop)

        schedule_(at, loop)


def loop(beats: int, track: int, align: int) -> callable:

    def wrapper(func):
        """
        """
        name = func.__name__

        if name not in loop_registry_:
            ll = Loop_(name, beats, track, align, func)

            ll.run()

            loop_registry_[name] = ll
        else:
            ll = loop_registry_[name]

            # We allow to update the beats and align of the loop,
            # however it will only be taken into account at the next
            # iteration of the loop.
            ll.beats = beats
            ll.align = align
            ll.track = track

            # Here is where we perform code update seamlessly.
            ll.func = func

        # This is a bit counter-intuitive: we don't allow to execute
        # the decorated function directly, it is scheduled the moment
        # the @loop decorator is called at definition time. We could
        # maybe raise an exception here to help the user, but at the
        # moment we don't have an error notification flow per-user.

        return lambda: None

    return wrapper


def assert_in_loop() -> Loop_:
    """Assert that we are in a loop

    Raises:
        NotInLoopException: If we are not in a loop.

    Returns:
        The current loop.
    """
    if not current_loop_:
        raise NotInLoopException()
    return current_loop_


def assert_not_in_loop():
    """Assert that we are not in a loop context.

    Raises:
        InLoopException: If we are in a loop.
    """
    if current_loop_:
        raise InLoopException()


def current_loop() -> Loop_:
    """Get the current loop.

    Returns:
        The current loop.
    """
    return current_loop_    


# Decorator API for @live


live_registry_ = dict()
current_live_ = None


class Live_:
    """Helper class to manage a live function.
    """
    def __init__(self, name: str, func: callable):
        self.name = name
        self.func = func

    def has_changed(self, func: callable) -> bool:
        """Check if the function has changed.

        Args:
            func: The function to check.

        Returns:
            True if the function has changed, False otherwise.
        """
        return inspect.getsource(func) != inspect.getsource(self.func)

    def run(self):
        """Executes right away the live function.
        """
        global current_live_

        current_live_ = self
        func = self.func
        func()
        current_live_ = None


def live() -> callable:

    def wrapper(func):
        """
        """
        name = func.__name__

        if name not in live_registry_:
            ll = Live_(name, func)
            ll.run()
            live_registry_[name] = ll
        else:            
            ll = live_registry_[name]
            if ll.has_changed(func):
                ll.func = func
                ll.run()

        # This is a bit counter-intuitive: we don't allow to execute
        # the decorated function directly, it is scheduled the moment
        # the @live decorator is called at definition time. We could
        # maybe raise an exception here to help the user, but at the
        # moment we don't have an error notification flow per-user.

        return lambda: None

    return wrapper


def current_live() -> Live_:
    """Get the current live.

    Returns:
        The current live.
    """
    return current_live_    


# Utilities API


def log(message: str) -> None:
    """Log a message to the console.
    """
    loop = current_loop()

    if loop:
        schedule_(loop.current_offset, lambda: log_(message))
    else:
        log_(message)


def sleep(beats: float):
    """Sleep for the duration of the current loop.
    """
    assert_in_loop().current_offset += beats

