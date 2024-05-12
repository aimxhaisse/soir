"""*Exception is not the rule.*

The errors module contains exceptions raised by the Neon engine to
signal errors at runtime.
"""


class NeonException(Exception):
    """Base class for Neon exceptions.
    """
    pass


class NotInLiveLoopException(NeonException):
    """Raised when trying to execute code outside of a live loop.

    In Neon, some of the code is expected to be used from within a
    loop, for instance sleeping between two instructions. This
    exception is raised if such code is called from the global scope.
    """
    pass


class InLiveLoopException(NeonException):
    """Raised when trying to execute code inside a live loop.

    In Neon, some of the code is expected to be used from the global
    scope as it affects everything, for example, setting the BPM. This
    exception is raised if such code is called from a loop.
    """
    pass
