"""Engine manager for Soir TUI.

This module manages the Soir engine lifecycle in a thread-safe manner,
providing a clean interface for the TUI to interact with the C++ engine.
"""

import os
import threading
from pathlib import Path

import soir._bindings as bindings
import soir._bindings.logging as logging
import soir._bindings.rt as rt
from soir.config import Config
from soir.watcher import Watcher


class EngineManager:
    """Manages Soir engine lifecycle in a thread-safe manner.

    This class wraps the C++ engine bindings and provides thread-safe
    access for the Textual TUI, which runs in an async context.
    """

    def __init__(self, session_path: Path, verbose: bool = False):
        """Initialize the engine manager.

        Args:
            session_path: Path to the session directory
            verbose: Enable verbose logging
        """
        self.session_path = session_path
        self.verbose = verbose
        self.soir: bindings.Soir | None = None
        self.watcher: Watcher | None = None
        self.config: Config | None = None
        self._lock = threading.Lock()
        self._running = False

    def initialize(self) -> tuple[bool, str]:
        """Initialize the engine and watcher.

        Returns:
            Tuple of (success, message) where success is True if
            initialization succeeded, and message contains either
            a success message or error details.
        """
        with self._lock:
            try:
                os.chdir(str(self.session_path))

                cfg_path = "etc/config.json"
                log_path = "var/log"

                logging.init(log_path, max_files=25, verbose=self.verbose)
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

                self.watcher = Watcher(
                    self.config, lambda code: self.soir.update_code(code)
                )
                self.watcher.start()
                logging.info("Watcher started successfully")

                self._running = True
                return True, "Engine started successfully"

            except Exception as e:
                error_msg = f"Unexpected error during initialization: {e}"
                logging.error(error_msg)
                return False, error_msg

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
                except Exception:
                    pass

            self._running = False

    def get_runtime_info(self) -> dict[str, any]:
        """Get current runtime information from the engine.

        Returns:
            Dictionary containing:
                - tracks: List of track dictionaries
                - bpm: Current BPM (float)
                - output_devices: List of audio output device names
        """
        with self._lock:
            if not self._running or not self.soir:
                return {"tracks": [], "bpm": 0.0, "output_devices": []}

            try:
                tracks = rt.get_tracks_()
                bpm = rt.get_bpm_()
                devices = rt.get_audio_out_devices_()

                return {
                    "tracks": tracks if tracks else [],
                    "bpm": bpm if bpm else 0.0,
                    "output_devices": devices if devices else [],
                }
            except Exception as e:
                logging.error(f"Failed to get runtime info: {e}")
                return {"tracks": [], "bpm": 0.0, "output_devices": []}

    def is_running(self) -> bool:
        """Check if the engine is currently running.

        Returns:
            True if the engine is running, False otherwise
        """
        with self._lock:
            return self._running
