"""Log viewer widget for Soir TUI.

This module provides a scrolling log viewer that displays Soir engine
logs with syntax highlighting based on log level.
"""

import re

from textual.widgets import RichLog


class LogViewerWidget(RichLog):
    """Scrolling log viewer for Soir logs."""

    def __init__(self) -> None:
        """Initialize the log viewer widget."""
        super().__init__(
            highlight=True, markup=True, wrap=False, auto_scroll=True, max_lines=1000
        )

    def add_log_line(self, line: str) -> None:
        """Add a parsed log line with syntax highlighting.

        Args:
            line: Raw log line from the log file
        """
        parts = self._parse_log_line(line)

        if parts:
            timestamp, level, location, message = parts

            level_colors = {
                "INFO": "green",
                "WARNING": "yellow",
                "ERROR": "red",
                "FATAL": "bright_red",
            }
            color = level_colors.get(level, "white")

            formatted = (
                f"[dim]{timestamp}[/dim] "
                f"[{color}]{level}[/{color}] "
                f"[dim]{location}[/dim] "
                f"{message}"
            )

            self.write(formatted)
        else:
            self.write(line)

    def _parse_log_line(self, line: str) -> tuple[str, str, str, str] | None:
        """Parse log line into components.

        Args:
            line: Raw log line

        Returns:
            Tuple of (timestamp, level, location, message) or None if invalid
        """
        pattern = r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) (\w+) \[([^\]]+)\] (.+)"
        match = re.match(pattern, line)

        if match:
            return match.groups()

        return None
