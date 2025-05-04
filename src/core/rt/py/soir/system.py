"""

The **system** module provides access to low-level facilities
of the soir engine.

# Cookbook

## Get the current audio devices

``` python
devices = system.get_audio_devices()
```

# Reference
"""

from bindings import (
    exec_session_,
    get_audio_devices_,
)


def get_audio_devices() -> list[tuple[int, str]]:
    """Get the current audio devices.

    Returns:
        A list of audio devices.
    """
    return get_audio_devices_()


def exec_session(name: str) -> None:
    """Executes the current session.

    Returns:
        None
    """
    exec_session_(name)
