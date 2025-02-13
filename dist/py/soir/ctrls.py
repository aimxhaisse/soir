"""
The **ctrls** module contains facilities to control settings of soir
in real-time.

# Cookbook

```python
@live
def controls():
    ctrls.mk_lfo('[x1]', rate=0.5)
    ctrls.mk_lfo('[x2]', rate=2.5)
    ctrls.mk_lfo('[x3]', rate=0.7, intensity=0.2)
```
"""

import soir._ctrls


def mk_lfo(name: str, rate: float, intensity: float = 1.0) -> None:
    """Create a new LFO parameter.

    Args:
        name: The name of the parameter.
        rate: The rate of the LFO in seconds.
        intensity: The intensity of the LFO.

    Returns:
        A new LFO parameter.
    """
    soir._ctrls.LFO_(name, rate, intensity)


def layout() -> list[str]:
    """Get the list of all controls.

    Returns:
        A list of all control names.
    """
    return list(soir._ctrls.controls_registry_.keys())
