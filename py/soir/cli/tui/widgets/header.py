"""Header widget for Soir TUI.

This module provides the header widget that displays the application
name and version at the top of the TUI.
"""

from importlib.metadata import version

from textual.reactive import reactive
from textual.widgets import Static


class HeaderWidget(Static):
    """Header showing 'soir' on left and version on right."""

    streaming_url: reactive[str] = reactive("")

    def __init__(self) -> None:
        """Initialize the header widget."""
        super().__init__()
        try:
            self.version = version("Soir")
        except Exception:
            self.version = "unknown"

    def render(self) -> str:
        """Render header content with ASCII art block letters.

        Returns:
            Formatted header string with ASCII art and version
        """
        ascii_art = [
            "███████  ██████  ██ ██████ ",
            "██      ██    ██ ██ ██   ██",
            "███████ ██    ██ ██ ██████ ",
            "     ██ ██    ██ ██ ██   ██",
            "███████  ██████  ██ ██   ██",
        ]

        version_text = f"v{self.version}"
        width = self.size.width

        stream_right = (
            f"stream  {self.streaming_url}" if self.streaming_url else ""
        )

        lines = []
        for i, line in enumerate(ascii_art):
            if i == 0:
                padding = " " * max(0, width - len(line) - len(version_text) - 2)
                lines.append(
                    f"[#7ba3d1]{line}[/#7ba3d1]{padding}[#4a5878]{version_text}[/#4a5878]"
                )
            elif i == 1 and stream_right:
                padding = " " * max(0, width - len(line) - len(stream_right) - 2)
                lines.append(
                    f"[#7ba3d1]{line}[/#7ba3d1]{padding}"
                    f"[#4a5878]stream[/#4a5878]  [#7ba3d1]{self.streaming_url}[/#7ba3d1]"
                )
            else:
                lines.append(f"[#7ba3d1]{line}[/#7ba3d1]")

        return "\n".join(lines)
