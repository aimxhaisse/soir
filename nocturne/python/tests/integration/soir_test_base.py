"""
Base test infrastructure for Soir integration tests.

Provides SoirTestEngine class which wraps the Soir engine
and provides utilities for testing, including log file parsing
to detect notifications.
"""

import re
import time
from pathlib import Path
from typing import Any

from jinja2 import Environment, FileSystemLoader

from soir._core import Soir, logging


class SoirTestEngine:
    """
    Test wrapper for Soir engine with log file monitoring.

    This class allows tests to:
    - Initialize and control the Soir engine
    - Push code to the engine
    - Wait for specific log messages (notifications)

    Example:
        engine = SoirTestEngine(log_dir="/tmp/test_logs")
        engine.push_code("log('hello')")
        assert engine.wait_for_notification("hello")
        engine.stop()
    """

    def __init__(
        self, log_dir: str, config_overrides: dict[str, Any] | None = None
    ) -> None:
        """
        Initialize the test engine.

        Args:
            log_dir: Directory where log files will be written
            config_overrides: Optional dict to override template variables
                             (e.g., {"initial_bpm": 600, "block_size": 2048})
        """
        self.log_dir = Path(log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)

        self.soir = Soir()
        self._log_file_path: Path | None = None
        self._last_read_position = 0
        self._notifications: list[str] = []
        self._notification_index = 0

        logging.init(str(self.log_dir), max_files=100, verbose=False)

        config_file = self._generate_config(config_overrides or {})
        if not self.soir.init(str(config_file)):
            raise RuntimeError("Failed to initialize Soir with generated config")

        if not self.soir.start():
            raise RuntimeError("Failed to start Soir engine")

        self._find_log_file()

    def _generate_config(self, overrides: dict[str, Any]) -> Path:
        """Generate a config file from the Jinja2 template."""
        template_dir = Path(__file__).parent
        env = Environment(loader=FileSystemLoader(template_dir))
        template = env.get_template("test_config.template.json")

        config_content = template.render(**overrides)

        config_file = self.log_dir / "config.json"
        config_file.write_text(config_content)

        return config_file

    def _find_log_file(self) -> None:
        """Find the most recent log file in the log directory."""
        log_files = sorted(
            self.log_dir.glob("soir.*.log"),
            key=lambda p: p.stat().st_mtime,
            reverse=True,
        )

        if log_files:
            self._log_file_path = log_files[0]
        else:
            raise RuntimeError(f"No log file found in {self.log_dir}")

    def _read_new_log_lines(self) -> list[str]:
        """Read new lines from the log file since last read."""
        if not self._log_file_path or not self._log_file_path.exists():
            return []

        try:
            with open(self._log_file_path, "r") as f:
                f.seek(self._last_read_position)
                new_lines = f.readlines()
                self._last_read_position = f.tell()
                return new_lines
        except Exception as e:
            print(f"Error reading log file: {e}")
            return []

    def push_code(self, code: str) -> bool:
        """Push Python code to the engine for execution."""
        return bool(self.soir.update_code(code))

    def get_notifications(self) -> list[str]:
        """Get all captured notifications."""
        return self._notifications

    def wait_for_notification(
        self, expected: str, timeout: float = 5.0, exact_match: bool = False
    ) -> bool:
        """
        Wait for a log message containing the expected string.

        This method polls the log file for new entries and checks if any
        contain the expected string. It's designed to work with the log()
        function in Soir code.

        The log format is:
        YYYY-MM-DD HH:MM:SS SEVERITY [source:line] message

        Multiline messages have the prefix only on the first line, with
        continuation lines combined into a single notification.

        Args:
            expected: String to look for in log messages
            timeout: Maximum time to wait in seconds
            exact_match: If True, the message must exactly match.
                        If False (default), substring match is used.

        Returns:
            True if notification was found within timeout, False otherwise

        Example:
            engine.push_code("log('hello world')")
            assert engine.wait_for_notification("hello")
            assert engine.wait_for_notification("hello world", exact_match=True)
        """
        start_time = time.time()

        while time.time() - start_time < timeout:

            # Here we want to be able to capture multiple lines in
            # case there is a multi-line log (with \n). This is useful
            # if we want to log big things in python and be able to
            # match against it (for instance code updates).
            capture: list[str] = []
            for line in self._read_new_log_lines():
                line = line.rstrip()
                pattern = r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} (INFO|WARN|ERROR|DEBUG|TRACE) \[.+:\d+\] (.*)"
                m = re.search(pattern, line)
                if m:
                    if capture:
                        self._notifications.append("\n".join(capture))
                        capture = []
                    capture.append(m.group(2))
                else:
                    capture.append(line)

            # We assume here that multiline logs are instantaneous, it
            # could happen that it takes more time to flush them and
            # _read_new_log_lines did not receive the full content
            # which we'll insert here. The alternative is to hang
            # until there is a new pattern-matching log.
            if capture:
                self._notifications.append("\n".join(capture))

            while self._notification_index < len(self._notifications):
                message = self._notifications[self._notification_index]
                self._notification_index += 1

                if exact_match:
                    if message == expected:
                        return True
                else:
                    if expected in message:
                        return True

            time.sleep(0.01)

        return False

    def stop(self) -> None:
        """Stop the Soir engine and clean up resources."""
        if not self.soir.stop():
            raise RuntimeError("Failed to stop Soir engine")
        del self.soir
        logging.shutdown()
