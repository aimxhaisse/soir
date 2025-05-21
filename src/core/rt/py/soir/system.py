"""

The **system** module provides access to low-level facilities
of the soir engine.

# Cookbook

## Get the current audio output devices

``` python
audio_out = system.get_audio_out_devices()
```

## Get the current audio output devices

``` python
audio_in = system.get_audio_in_devices()
```

## Get the current MIDI output devices

``` python
midi_out = system.get_midi_out_devices()
```

# Reference
"""

from bindings import (
    exec_session_,
    get_audio_in_devices_,
    get_audio_out_devices_,
    get_midi_out_devices_,
)


def get_audio_out_devices() -> list[tuple[int, str]]:
    """Get the current output audio devices.

    Returns:
        A list of audio output devices.
    """
    return get_audio_out_devices_()


def get_audio_in_devices() -> list[tuple[int, str]]:
    """Get the current input audio devices.

    Returns:
        A list of audio input devices.
    """
    return get_audio_in_devices_()


def get_midi_out_devices() -> list[tuple[int, str]]:
    """Get the current MIDI output devices.

    Returns:
        A list of MIDI output devices.
    """
    return get_midi_out_devices_()


def exec_session(name: str) -> None:
    """Executes the current session.

    Returns:
        None
    """
    exec_session_(name)
