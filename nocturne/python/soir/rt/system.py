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

## Sets the force kill at exit
``` python
system.set_force_kill_at_shutdown(true)
```

## Prints the current informations about the system
``` python
system.info()
```

# Reference
"""

from soir._core.rt import (
    get_audio_in_devices_,
    get_audio_out_devices_,
    get_midi_out_devices_,
    set_force_kill_at_shutdown_,
)
from soir.rt._internals import (
    assert_not_in_loop,
    log,
)
from soir.rt._system import (
    record_,
)


def record(file_path: str) -> bool:
    """Record audio to a WAV file.

    This function starts recording all audio output to the specified WAV file.
    Recording will automatically stop if this function is not called in a
    subsequent evaluation cycle (similar to how controls work). This enables
    automatic cleanup when code is changed or removed.

    Args:
        file_path: Path to the WAV file where audio will be recorded.
                   The file will be created or overwritten if it exists.
                   Parent directories will be created automatically if needed.

    Returns:
        True if recording started/continued successfully, False otherwise.

    Raises:
        InLoopException: If called from within a loop context. Recording
                        must be initiated from the global scope only.
    """
    assert_not_in_loop()
    return record_(file_path)


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

    Args:
        name: str, the name of the session to execute.

    Returns:
        None
    """
    exec_session_(name)


def set_force_kill_at_shutdown(flag: bool) -> None:
    """Sends a kill signal to the Python thread at exit.

    This is required under rare circumstances where the Python thread
    is blocked (like when serving documentation).

    Args:
        flag: bool, whether to set the force kill flag at shutdown or not.

    Returns:
        None
    """
    set_force_kill_at_shutdown_(flag)


def info() -> None:
    """Prints the current layout of the system."""
    log("=== Audio output devices ===")
    audio_out = get_audio_out_devices()
    for idx, name in audio_out:
        log(f"  {idx}: {name}")
    log("=== Audio input devices ===")
    audio_in = get_audio_in_devices()
    for idx, name in audio_in:
        log(f"  {idx}: {name}")
    log("=== MIDI output devices ===")
    midi_out = get_midi_out_devices()
    for idx, name in midi_out:
        log(f"  {idx}: {name}")
