"""
The errors module contains exceptions raised by the Soir engine to
signal errors at runtime.
"""


class SoirException(Exception):
    """Base class for Soir exceptions.

    @public
    """

    pass


class NotInLoopException(SoirException):
    """Raised when trying to execute code outside of a live context.

    @public

    In Soir, some of the code is expected to be used from within a
    loop or a live function, for instance sleeping between two
    instructions. This exception is raised if such code is called from
    the global scope.
    """

    pass


class InLoopException(SoirException):
    """Raised when trying to execute code inside a live context.

    @public

    In Soir, some of the code is expected to be used from the global
    scope as it affects everything, for example, setting the BPM. This
    exception is raised if such code is called from a loop.
    """

    pass


class NotInControlLoopException(SoirException):
    """Raised when trying to use a control outside of a control loop.

    @public

    In Soir, controls are used to change settings in real-time and are
    orchestrated by the engine. This exception is raised if a control
    is instrumented from outside of the Soir control loop.
    """

    pass


class UnknownMidiTrackException(SoirException):
    """Raised when trying to use an invalid MIDI track.

    @public

    In Soir, MIDI tracks are used to send MIDI events to the external
    synthesizer. This exception is raised if an invalid MIDI track is
    used.
    """

    pass


class ControlNotFoundException(SoirException):
    """Raised when trying to use an unknown control.

    @public

    In Soir, controls are used to change settings in real-time and are
    orchestrated by the engine. This exception is raised if an undefined
    control is used.
    """

    pass


class ConfigurationError(SoirException):
    """Raised when there is a configuration error.

    @public

    This exception is raised when required configuration is missing,
    such as the SOIR_DIR environment variable not being set.
    """

    pass


class SamplePackNotFoundException(SoirException):
    """Raised when trying to load a non-existing sample pack.

    @public

    Sample packs are loaded once at the initialization of the session,
    trying to refer to a non-loaded pack triggers this
    exception. Configure your session to include the desired sample
    packs.
    """

    pass
