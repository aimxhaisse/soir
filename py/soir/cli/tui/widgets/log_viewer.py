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
                "INFO": "#5b9a6d",
                "WARNING": "#c9a857",
                "ERROR": "#c95757",
                "FATAL": "#ff6b6b",
            }
            color = level_colors.get(level, "#b8c0cc")

            formatted = (
                f"[#4a5878]{timestamp}[/#4a5878] "
                f"[{color}]{level}[/{color}] "
                f"[#4a5878]{location}[/#4a5878] "
                f"[#b8c0cc]{message}[/#b8c0cc]"
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
            timestamp, level, location, message = match.groups()
            return timestamp, level, location, message

        return None
