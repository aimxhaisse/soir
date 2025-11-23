from collections.abc import Callable

from soir._bindings.rt import (
    schedule_,
    get_beat_,
    log_,
    get_code_,
)
from soir.rt.errors import (
    InLoopException,
    NotInLoopException,
)


# Incremented at each buffer evaluation, used to remove loops and live
# functions that are not part of the buffer anymore.
eval_id_ = 0


def _reset() -> None:
    """Helper to reset internals state for integration tests."""
    global eval_id_

    eval_id_ = 0

    _reset_loop()
    _reset_live()


# Decorator API for @loop


loop_registry_: dict[str, "Loop_"] = {}
current_loop_: "Loop_ | None" = None


def _reset_loop() -> None:
    """Helper to reset loop state for integration tests."""
    global current_loop_, loop_registry_

    current_loop_ = None
    loop_registry_.clear()


class Loop_:
    """Helper class to manage a loop function."""

    def __init__(
        self,
        name: str,
        beats: int,
        track: str | None,
        align: bool,
        func: Callable[..., None],
    ):
        self.name = name
        self.beats = beats
        self.track = track
        self.align = align
        self.func = func
        self.updated_at: int | None = None
        self.eval_at: int | None = None

        self.extra: dict[str, int | str] = {}

        # Current offset in beats in the loop, used to schedule events
        # in the future.
        self.current_offset: float = 0

    def run(self) -> None:
        """Temporal recursion scheduling of the loop."""
        now = get_beat_()
        at = 0
        if self.align:
            at = self.beats - (now % self.beats)

        def _loop() -> None:
            if self.name not in loop_registry_:
                return

            global current_loop_

            err = None
            current_loop_ = self
            self.current_offset = 0
            try:
                self.eval_at = get_beat_()
                self.func()
            except Exception as e:
                err = e
            current_loop_ = None
            schedule_(self.beats, _loop)

            # We raise after rescheduling the loop, this helps making
            # sure we don't have dead loops if an error is raised
            # somewhere. The downside is we'll probably make noise
            # anyway.
            if err:
                raise err

        schedule_(at, _loop)


def loop(
    track: str | None, beats: int, align: bool
) -> Callable[[Callable[..., None]], Callable[..., None]]:

    def wrapper(func: Callable[..., None]) -> Callable[..., None]:
        name = func.__name__

        if name not in loop_registry_:
            ll = Loop_(name, beats, track, align, func)

            ll.updated_at = eval_id_
            ll.run()

            log(f"adding loop {name}")
            loop_registry_[name] = ll
        else:
            ll = loop_registry_[name]

            ll.updated_at = eval_id_

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
    if current_loop_ is None:
        raise NotInLoopException()
    return current_loop_


def assert_not_in_loop() -> None:
    """Assert that we are not in a loop context.

    Raises:
        InLoopException: If we are in a loop.
    """
    if current_loop_:
        raise InLoopException()


def current_loop() -> Loop_ | None:
    """Get the current loop.

    Returns:
        The current loop.
    """
    return current_loop_


def get_loop(name: str) -> Loop_ | None:
    """Get the loop by name.

    Args:
        name: The name of the loop function.

    Returns:
        The loop function.
    """
    return loop_registry_.get(name)


# Decorator API for @live


live_registry_: dict[str, "Live_"] = {}
current_live_: "Live_ | None" = None


def _reset_live() -> None:
    """Helper to reset live state for integration tests."""
    global current_live_, live_registry_

    current_live_ = None
    live_registry_.clear()


def get_code_function_(func: Callable[..., None]) -> str:
    """Get the code of a function.

    This is a helper function to get the code of a function. In Soir
    the code is not executed from a file, but from a buffer, so
    inspect.getsource() won't work as it expects a global __file__
    properly set. We could maybe later support this by having a
    temporary file if needed. Instead wwe use co_lines which is
    properly set from the buffer and infer the actual code from there.

    This is meant to know when to re-execute a @live loop: if its code
    changed between two buffer evaluations.

    Args:
        func: The function to get the code from.

    Returns:
        The code of the function.
    """
    start_line = None
    end_line = None
    for _, _, line in func.__code__.co_lines():
        if not line:
            continue
        if not start_line:
            start_line = line
        if not end_line or end_line < line:
            end_line = line
    return "\n".join(get_code_().splitlines()[start_line:end_line])


class Live_:
    """Helper class to manage a live function."""

    def __init__(self, name: str, func: Callable[..., None], code: str):
        self.name = name
        self.func = func
        self.code = code
        self.updated_at: int | None = None
        self.eval_at: int | None = None

    def run(self) -> None:
        """Executes right away the live function."""
        global current_live_

        err = None
        current_live_ = self
        func = self.func
        try:
            func()
        except Exception as e:
            err = e
        current_live_ = None
        if err:
            raise err


def live() -> Callable[[Callable[..., None]], Callable[..., None]]:

    def wrapper(func: Callable[..., None]) -> Callable[..., None]:
        name = func.__name__
        ll = live_registry_.get(name)
        code = get_code_function_(func)

        if not ll:
            ll = Live_(name, func, code)
            live_registry_[name] = ll
            ll.updated_at = eval_id_
            ll.eval_at = get_beat_()
            ll.run()
        else:
            ll.updated_at = eval_id_
            if ll.code != code:
                ll.func = func
                ll.code = code
                ll.eval_at = get_beat_()
                ll.run()

        # This is a bit counter-intuitive: we don't allow to execute
        # the decorated function directly, it is scheduled the moment
        # the @live decorator is called at definition time. We could
        # maybe raise an exception here to help the user, but at the
        # moment we don't have an error notification flow per-user.

        return lambda: None

    return wrapper


def current_live() -> Live_ | None:
    """Get the current live.

    Returns:
        The current live.
    """
    return current_live_


def get_live(name: str) -> Live_ | None:
    """Get the live function by name.

    Args:
        name: The name of the live function.

    Returns:
        The live function.
    """
    return live_registry_.get(name)


## Internal facilities


def post_eval_() -> None:
    """Called after each buffer evaluation.

    Currently used to clean-up loops and live functions that are not
    part of the buffer anymore.
    """
    global eval_id_

    loops_to_remove = []
    for name, ll in loop_registry_.items():
        if ll.updated_at != eval_id_:
            loops_to_remove.append(name)
    for name in loops_to_remove:
        log(f"removing loop {name}")
        del loop_registry_[name]

    live_to_remove = []
    for name, lv in live_registry_.items():
        if lv.updated_at != eval_id_:
            live_to_remove.append(name)
    for name in live_to_remove:
        del live_registry_[name]
        log(f"removing live {name}")

    eval_id_ += 1


# Utilities API


def log(message: str) -> None:
    """Log a message to the console."""
    loop = current_loop()

    if loop:
        schedule_(loop.current_offset, lambda: log_(message))
    else:
        log_(message)


def sleep(beats: float) -> None:
    """Sleep for the duration of the current loop."""
    assert_in_loop().current_offset += beats
