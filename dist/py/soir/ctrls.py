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
    sp.play('my-sample', pan=ctrl('[x0]')
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
    create specific controls:

    - mk_lfo()
    - mk_linear()

    Once a control is created, it can be referred to by using the
    `ctrl('name')` facility and can passed as a parameter to
    instrument calls or FXs.
    """
    def __init__(self) -> None:
        raise NotImplementedError()

    def set(self, **params) -> None:
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


def mk_lfo(name: str, rate: float, intensity: float = 1.0) -> None:
    """Create a new LFO parameter.

    Args:
        name: The name of the parameter.
        rate: The rate of the LFO in seconds.
        intensity: The intensity of the LFO.
    """
    soir._ctrls.LFO_(name, rate, intensity)


def mk_linear(name: str, start: float, end: float, duration: float) -> None:
    """Create a new linear parameter.

    Args:
        name: The name of the parameter.
        start: The start value.
        end: The end value.
        duration: The duration of the transition in seconds.
    """
    soir._ctrls.Linear_(name, start, end, duration)
    

def layout() -> list[Control]:
    """Get the list of all controls.

    Returns:
        A list of all controls.
    """
    return list(soir._ctrls.controls_registry_.values())
