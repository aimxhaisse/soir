"""Header widget for Soir TUI.

This module provides the header widget that displays the application
name and version at the top of the TUI.
"""

from importlib.metadata import version

from textual.widgets import Static


class HeaderWidget(Static):
    """Header showing 'soir' on left and version on right."""

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

        lines = []
        for i, line in enumerate(ascii_art):
            if i == 0:
                padding = " " * max(0, width - len(line) - len(version_text) - 2)
                lines.append(
                    f"[#7ba3d1]{line}[/#7ba3d1]{padding}[#4a5878]{version_text}[/#4a5878]"
                )
            else:
                lines.append(f"[#7ba3d1]{line}[/#7ba3d1]")

        return "\n".join(lines)
