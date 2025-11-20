"""
Base test infrastructure for Soir integration tests.

Provides SoirTestEngine class which wraps the Soir engine
and provides utilities for testing, including log file parsing
to detect notifications.
"""

import re
import time
from pathlib import Path

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

    def __init__(self, log_dir: str, config_overrides: dict | None = None):
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

        logging.init(str(self.log_dir), max_files=100, verbose=False)

        config_file = self._generate_config(config_overrides or {})
        if not self.soir.init(str(config_file)):
            raise RuntimeError("Failed to initialize Soir with generated config")

        if not self.soir.start():
            raise RuntimeError("Failed to start Soir engine")

        self._find_log_file()

    def _generate_config(self, overrides: dict) -> Path:
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
            reverse=True
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
            with open(self._log_file_path, 'r') as f:
                f.seek(self._last_read_position)
                new_lines = f.readlines()
                self._last_read_position = f.tell()
                return new_lines
        except Exception as e:
            print(f"Error reading log file: {e}")
            return []

    def push_code(self, code: str) -> bool:
        """Push Python code to the engine for execution."""
        return self.soir.update_code(code)

    def wait_for_notification(
        self,
        expected: str,
        timeout: float = 5.0,
        exact_match: bool = False
    ) -> bool:
        """
        Wait for a log message containing the expected string.

        This method polls the log file for new entries and checks if any
        contain the expected string. It's designed to work with the log()
        function in Soir code.

        The log format is:
        YYYY-MM-DD HH:MM:SS SEVERITY [source:line] message

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
            new_lines = self._read_new_log_lines()

            for line in new_lines:
                match = re.search(r'\[.*?:\d+\]\s+(.*)$', line)
                if match:
                    message = match.group(1).strip()

                    if exact_match:
                        if message == expected:
                            return True
                    else:
                        if expected in message:
                            return True

            time.sleep(0.01)

        return False

    def wait_for_multiple_notifications(
        self,
        expected_list: list[str],
        timeout: float = 5.0
    ) -> bool:
        """Wait for multiple notifications in order."""
        for expected in expected_list:
            if not self.wait_for_notification(expected, timeout):
                return False
        return True

    def stop(self) -> None:
        """Stop the Soir engine and clean up resources."""
        if not self.soir.stop():
            raise RuntimeError("Failed to stop Soir engine")

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - automatically stops engine."""
        self.stop()
