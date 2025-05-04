"""
The **ctrls** module contains facilities to control settings of soir
in real-time.

# Cookbook

```python
@live()
def controls():
    ctrls.mk_lfo('[x0]', rate=0.5, intensity=0.75)

sp = samples.new('my-pack')

@loop()
def play():
    sp.play('my-sample', pan=ctrl('[x0]'))
```
"""

import soir._ctrls


class Control(soir._ctrls.Control_):
    """Base class for a control.

    A control computes a value to the Soir engine about 100 times per
    second via the `Control.fwd()` call, this value is then
    interpolated by the C++ engine to provide a smooth transition
    between values.

    The Control class is not meant to be created directly unless you
    want to implement your own control, helpers are available to
    create specific controls (see below).

    Once a control is created, it can be referred to by using the
    `ctrl('name')` facility and can passed as a parameter to
    instrument calls or FXs.
    """

    def __init__(self) -> None:
        raise NotImplementedError()

    def name(self) -> str:
        """Get the name of the control.

        Returns:
            The name of the control.
        """
        raise NotImplementedError()

    def set(self, **params: float) -> None:
        """Set the control parameters.

        Args:
            **params: The parameters to set.
        """
        raise NotImplementedError()

    def get(self) -> float:
        """Get the current value of the control.

        Returns:
                The current value of the control.
        """
        raise NotImplementedError()

    def fwd(self):
        """Computes the next value of the control and advance the tick.

        This is meant to be used by the soir engine.
        """
        raise NotImplementedError()

    def __repr__(self) -> str:
        """Get the string representation of the control."""
        return f"Control(name={self.name()})"


def mk_lfo(
    name: str, rate: float, intensity: float = 1.0, low: float = -1.0, high: float = 1.0
) -> None:
    """Create a new LFO parameter.

    Args:
        name: The name of the parameter.
        rate: The rate of the LFO in seconds.
        intensity: The intensity of the LFO.
        low: The minimum value of the LFO (defaults to -1.0).
        high: The maximum value of the LFO (defaults to 1.0).
    """
    soir._ctrls.LFO_(name, rate, intensity, low, high)


def mk_linear(name: str, start: float, end: float, duration: float) -> None:
    """Create a new linear parameter.

    Args:
        name: The name of the parameter.
        start: The start value.
        end: The end value.
        duration: The duration of the transition in seconds.
    """
    soir._ctrls.Linear_(name, start, end, duration)


def mk_val(name: str, value: float) -> None:
    """Create a new value parameter.

    Args:
        name: The name of the parameter.
        value: The value.
    """
    soir._ctrls.Val_(name, value)


def mk_func(name: str, func: callable) -> None:
    """Create a new function parameter.

    Args:
        name: The name of the parameter.
        func: The function to compute the value.
    """
    soir._ctrls.Func_(name, func)


def layout() -> list[Control]:
    """Get the list of all controls.

    Returns:
        A list of all controls.
    """
    return list(soir._ctrls.controls_registry_.values())
