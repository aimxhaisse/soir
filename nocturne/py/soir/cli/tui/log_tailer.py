"""Log tailer for Soir TUI.

This module provides log file tailing functionality, monitoring the
Soir log directory for new log entries and delivering them to a callback.
"""

import time
from collections.abc import Callable
from pathlib import Path


class LogTailer:
    """Tails Soir log files and provides new lines via callback.

    This class monitors the log directory for the most recent log file
    and reads new lines as they appear, delivering them to a callback
    function for display in the TUI.
    """

    def __init__(self, log_dir: Path, callback: Callable[[str], None]):
        """Initialize the log tailer.

        Args:
            log_dir: Directory containing log files
            callback: Function to call with each new log line
        """
        self.log_dir = log_dir
        self.callback = callback
        self.current_file: Path | None = None
        self._running = False
        self._file_pos = 0

    def start(self) -> None:
        """Start tailing the most recent log file.

        This method runs in a loop, continuously monitoring for new
        log entries and calling the callback with each new line.
        This is designed to be called from a Textual worker thread.
        """
        self._running = True

        while self._running:
            try:
                log_file = self._find_latest_log()

                if log_file != self.current_file:
                    self.current_file = log_file
                    self._file_pos = 0

                if self.current_file and self.current_file.exists():
                    self._tail_file(self.current_file)

                time.sleep(0.1)
            except Exception:
                time.sleep(1.0)

    def stop(self) -> None:
        """Stop tailing log files."""
        self._running = False

    def _find_latest_log(self) -> Path | None:
        """Find the most recent log file in the log directory.

        Returns:
            Path to the most recent log file, or None if no logs found
        """
        if not self.log_dir.exists():
            return None

        log_files = list(self.log_dir.glob("soir.*.log"))
        if not log_files:
            return None

        return max(log_files, key=lambda p: p.stat().st_mtime)

    def _tail_file(self, file_path: Path) -> None:
        """Read new lines from the log file.

        Args:
            file_path: Path to the log file to tail
        """
        try:
            with open(file_path, "r") as f:
                f.seek(self._file_pos)

                lines = f.readlines()
                self._file_pos = f.tell()

                for line in lines:
                    self.callback(line.rstrip())

        except FileNotFoundError:
            pass
        except Exception:
            pass
