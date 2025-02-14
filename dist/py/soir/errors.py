"""
The errors module contains exceptions raised by the Soir engine to
signal errors at runtime.
"""


class SoirException(Exception):
    """Base class for Soir exceptions.
    """
    pass


class NotInLoopException(SoirException):
    """Raised when trying to execute code outside of a live context.

    In Soir, some of the code is expected to be used from within a
    loop or a live function, for instance sleeping between two
    instructions. This exception is raised if such code is called from
    the global scope.
    """
    pass


class InLoopException(SoirException):
    """Raised when trying to execute code inside a live context.

    In Soir, some of the code is expected to be used from the global
    scope as it affects everything, for example, setting the BPM. This
    exception is raised if such code is called from a loop.
    """
    pass


class NotInControlLoopException(SoirException):
    """Raised when trying to use a control outside of a control loop.

    In Soir, controls are used to change settings in real-time and are
    orchestrated by the engine. This exception is raised if a control
    is instrumented from outside of the Soir control loop.
    """
    pass


class UnknownMidiTrackException(SoirException):
    """Raised when trying to use an invalid MIDI track.

    In Soir, MIDI tracks are used to send MIDI events to the external
    synthesizer. This exception is raised if an invalid MIDI track is
    used.
    """
    pass


class ControlNotFoundException(SoirException):
    """Raised when trying to use an unknown control.

    In Soir, controls are used to change settings in real-time and are
    orchestrated by the engine. This exception is raised if an undefined
    control is used.
    """
    pass
