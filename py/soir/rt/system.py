"""
The **system** module provides access to low-level facilities
of the soir engine.

@public
"""

from dataclasses import dataclass

from soir._bindings.rt import (
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


@dataclass
class AudioDeviceInfo:
    """Information about an audio device.

    @public

    Attributes:
        id: Device index.
        name: Device name.
        is_default: Whether this is the system default device.
        channels: Number of channels supported by the device.
    """

    id: int
    name: str
    is_default: bool
    channels: int


def record(file_path: str) -> bool:
    """Record audio to a WAV file.

    @public

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


def get_audio_out_devices() -> list[AudioDeviceInfo]:
    """Get the current output audio devices.

    @public

    Returns:
        A list of audio output devices with detailed information.
    """
    raw = get_audio_out_devices_()
    return [AudioDeviceInfo(**d) for d in raw]


def get_audio_in_devices() -> list[AudioDeviceInfo]:
    """Get the current input audio devices.

    @public

    Returns:
        A list of audio input devices with detailed information.
    """
    raw = get_audio_in_devices_()
    return [AudioDeviceInfo(**d) for d in raw]


def get_midi_out_devices() -> list[tuple[int, str]]:
    """Get the current MIDI output devices.

    @public

    Returns:
        A list of MIDI output devices.
    """
    result: list[tuple[int, str]] = get_midi_out_devices_()
    return result


def set_force_kill_at_shutdown(flag: bool) -> None:
    """Sends a kill signal to the Python thread at exit.

    @public

    This is required under rare circumstances where the Python thread
    is blocked (like when serving documentation).

    Args:
        flag: bool, whether to set the force kill flag at shutdown or not.

    Returns:
        None
    """
    set_force_kill_at_shutdown_(flag)


def info() -> None:
    """Prints the current layout of the system.

    @public

    Returns:
        None
    """
    log("=== Audio output devices ===")
    audio_out = get_audio_out_devices()
    for dev in audio_out:
        default_marker = " *" if dev.is_default else ""
        log(f"  {dev.id}: {dev.name}{default_marker} ({dev.channels}ch)")
    log("=== Audio input devices ===")
    audio_in = get_audio_in_devices()
    for dev in audio_in:
        default_marker = " *" if dev.is_default else ""
        log(f"  {dev.id}: {dev.name}{default_marker} ({dev.channels}ch)")
    log("=== MIDI output devices ===")
    midi_out = get_midi_out_devices()
    for idx, name in midi_out:
        log(f"  {idx}: {name}")
