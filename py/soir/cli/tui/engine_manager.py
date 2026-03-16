"""Engine manager for Soir TUI.

This module manages the Soir engine lifecycle in a thread-safe manner,
providing a clean interface for the TUI to interact with the C++ engine.
"""

import os
import sys
import threading
from pathlib import Path
from typing import Any

import soir._bindings as bindings
from soir._bindings import logging, rt
from soir.config import Config, get_soir_dir, is_session
from soir.watcher import Watcher

_STDOUT_FD = 1
_STDERR_FD = 2


def redirect_python_io_to_tty() -> None:
    # Before we call logging.init(redirect_stdio=True), the C++ side will
    # freopen() fd 1 and fd 2 to the log file. Python's sys.stdout holds
    # fd 1 directly, so any print() after that point would silently go to
    # the log file. We open a new file descriptor pointing to the real
    # terminal (/dev/tty on Linux/macOS, CONOUT$ on Windows) and swap it
    # in BEFORE the freopen happens, so Python I/O remains on the terminal.
    # When there is no controlling terminal (e.g. in tests or CI) the open
    # will fail with OSError — we skip the redirect safely in that case.
    #
    # This must be called before any framework (e.g. Textual) captures
    # sys.__stdout__, otherwise the framework will hold a reference to fd 1
    # and its output will be redirected to the log file when freopen() runs.
    # If stdout was already redirected away from fd 1 (idempotency guard),
    # skip to avoid opening a second tty handle.
    try:
        if sys.stdout.fileno() != _STDOUT_FD:
            return
    except (AttributeError, OSError):
        pass
    _tty_path = "CONOUT$" if sys.platform == "win32" else "/dev/tty"
    try:
        # Intentionally not using a context manager: the file must stay open
        # for the entire process lifetime so Python output keeps reaching the
        # terminal after fd 1/2 are redirected to the log file.
        _tty = open(_tty_path, "w")  # noqa: SIM115
    except OSError:
        return
    sys.stdout = _tty
    sys.stderr = _tty
    # mypy treats __stdout__/__stderr__ as Final, but reassigning them is valid
    # Python and necessary to keep the canonical stream references in sync with
    # the redirected stdout/stderr (e.g. for code that reads sys.__stdout__).
    sys.__stdout__ = _tty  # type: ignore[misc]
    sys.__stderr__ = _tty  # type: ignore[misc]


class EngineManager:
    """Manages Soir engine lifecycle in a thread-safe manner.

    This class wraps the C++ engine bindings and provides thread-safe
    access for the Textual TUI, which runs in an async context.
    """

    def __init__(self, verbose: bool = False):
        """Initialize the engine manager.

        Args:
            verbose: Enable verbose logging
        """
        self.verbose = verbose
        self.soir: bindings.Soir | None = None
        self.watcher: Watcher | None = None
        self.config: Config | None = None
        self._lock = threading.Lock()
        self._running = False
        self._saved_stdout_fd: int | None = None
        self._saved_stderr_fd: int | None = None

    def initialize_session(self, session_path: Path) -> tuple[bool, str]:
        """Initialize the engine and watcher from a session directory.

        Changes the working directory to session_path and starts the
        file watcher for live coding if the path is a session (not SOIR_DIR).

        Args:
            session_path: Path to the session directory containing
                etc/config.json and var/log/.

        Returns:
            Tuple of (success, message) where success is True if
            initialization succeeded, and message contains either
            a success message or error details.
        """
        with self._lock:
            try:
                os.chdir(str(session_path))

                cfg_path = "etc/config.json"
                log_path = "var/log"

                redirect_python_io_to_tty()
                logging.init(
                    log_path, max_files=25, verbose=self.verbose, redirect_stdio=True
                )
                logging.info("Starting Soir live session")

                if not os.path.exists(cfg_path):
                    error_msg = f"Config file not found at {cfg_path}"
                    logging.error(error_msg)
                    return False, error_msg

                self.config = Config.load_from_path(cfg_path)
                self.soir = bindings.Soir()

                if not self.soir.init(cfg_path):
                    error_msg = "Failed to initialize Soir engine"
                    logging.error(error_msg)
                    return False, error_msg

                if not self.soir.start():
                    error_msg = "Failed to start Soir engine"
                    logging.error(error_msg)
                    return False, error_msg

                logging.info("Soir started successfully")

                if is_session(session_path):
                    soir_engine = self.soir
                    self.watcher = Watcher(
                        self.config, lambda code: soir_engine.update_code(code)
                    )
                    self.watcher.start()
                    logging.info("Watcher started successfully")

                self._running = True

            except Exception as e:  # noqa: BLE001
                error_msg = f"Unexpected error during initialization: {e}"
                logging.error(error_msg)
                return False, error_msg
            else:
                return True, "Engine started successfully"

    def initialize_standalone(self) -> tuple[bool, str]:
        """Initialize the engine from the global SOIR_DIR configuration.

        Does not change the working directory and does not start a watcher.
        Intended for one-shot CLI commands that need engine access outside
        of any session context.

        Returns:
            Tuple of (success, message) where success is True if
            initialization succeeded, and message contains either
            a success message or error details.
        """
        with self._lock:
            try:
                soir_dir = Path(get_soir_dir())
                cfg_path = str(soir_dir / "etc" / "config.json")
                log_path = str(soir_dir / "var" / "log")

                Path(log_path).mkdir(parents=True, exist_ok=True)
                self._saved_stdout_fd = os.dup(_STDOUT_FD)
                self._saved_stderr_fd = os.dup(_STDERR_FD)
                logging.init(
                    log_path, max_files=25, verbose=self.verbose, redirect_stdio=True
                )

                if not os.path.exists(cfg_path):
                    error_msg = f"Config file not found at {cfg_path}"
                    return False, error_msg

                self.config = Config.load_from_path(cfg_path)
                self.soir = bindings.Soir()

                if not self.soir.init(cfg_path):
                    error_msg = "Failed to initialize Soir engine"
                    return False, error_msg

                if not self.soir.start():
                    error_msg = "Failed to start Soir engine"
                    return False, error_msg

                self._running = True

            except Exception as e:  # noqa: BLE001
                error_msg = f"Unexpected error during initialization: {e}"
                return False, error_msg
            else:
                return True, "Engine started successfully"

    def stop(self) -> None:
        """Stop the engine and watcher."""
        with self._lock:
            if self.watcher:
                self.watcher.stop()
                self.watcher = None

            if self.soir:
                self.soir.stop()
                self.soir = None

            if self._running:
                try:
                    logging.shutdown()
                except Exception:  # noqa: BLE001, S110
                    pass

            if self._saved_stdout_fd is not None:
                os.dup2(self._saved_stdout_fd, _STDOUT_FD)
                os.close(self._saved_stdout_fd)
                self._saved_stdout_fd = None
            if self._saved_stderr_fd is not None:
                os.dup2(self._saved_stderr_fd, _STDERR_FD)
                os.close(self._saved_stderr_fd)
                self._saved_stderr_fd = None

            self._running = False

    def get_runtime_info(self) -> dict[str, Any]:
        """Get current runtime information from the engine.

        Returns:
            Dictionary containing:
                - tracks: List of track dictionaries
                - bpm: Current BPM (float)
                - output_devices: List of audio output device names
        """
        with self._lock:
            if not self._running or not self.soir:
                return {"tracks": [], "bpm": 0.0, "beat": 0.0, "output_devices": []}

            try:
                tracks = rt.get_tracks_()
                bpm = rt.get_bpm_()
                beat = rt.get_beat_()
                devices = rt.get_audio_out_devices_()
            except Exception as e:  # noqa: BLE001
                logging.error(f"Failed to get runtime info: {e}")
                return {"tracks": [], "bpm": 0.0, "beat": 0.0, "output_devices": []}
            else:
                return {
                    "tracks": tracks if tracks else [],
                    "bpm": bpm if bpm else 0.0,
                    "beat": beat if beat else 0.0,
                    "output_devices": devices if devices else [],
                }

    def set_audio_output_device(self, device: str) -> tuple[bool, str]:
        """Set the audio output device and persist to config.

        Args:
            device: "none" to disable, "default" for system default, or a
                device name substring for a specific device.

        Returns:
            Tuple of (success, message)
        """
        with self._lock:
            if not self._running:
                return False, "engine not running"
            assert self.config is not None
            self.config.dsp.audio_output_device = device
            try:
                self.config.save_to_path("etc/config.json")
            except OSError as e:
                return False, f"failed to write config: {e}"
            success = rt.set_audio_out_device_(device)
            if not success:
                return False, "engine rejected device"
            return True, f"audio output: {device}"

    def is_running(self) -> bool:
        """Check if the engine is currently running.

        Returns:
            True if the engine is running, False otherwise
        """
        with self._lock:
            return self._running
