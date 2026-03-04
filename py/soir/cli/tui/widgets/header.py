"""Header widget for Soir TUI.

This module provides the header widget that displays the application
name and version at the top of the TUI.
"""

from importlib.metadata import PackageNotFoundError, version

from textual.reactive import reactive
from textual.widgets import Static


class HeaderWidget(Static):
    """Header showing 'soir' on left and version on right."""

    streaming_url: reactive[str] = reactive("")
    cast_url: reactive[str] = reactive("")

    def __init__(self) -> None:
        """Initialize the header widget."""
        super().__init__()
        try:
            self.version = version("Soir")
        except PackageNotFoundError:
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

        if self.cast_url:
            url_label, url_value = "cast", self.cast_url
        elif self.streaming_url:
            url_label, url_value = "stream", self.streaming_url
        else:
            url_label, url_value = "", ""

        url_right = f"{url_label}  {url_value}" if url_value else ""

        lines = []
        for i, line in enumerate(ascii_art):
            if i == 0:
                padding = " " * max(0, width - len(line) - len(version_text) - 2)
                lines.append(
                    f"[#7ba3d1]{line}[/#7ba3d1]{padding}[#4a5878]{version_text}[/#4a5878]"
                )
            elif i == 1 and url_right:
                padding = " " * max(0, width - len(line) - len(url_right) - 2)
                lines.append(
                    f"[#7ba3d1]{line}[/#7ba3d1]{padding}"
                    f"[#4a5878]{url_label}[/#4a5878]  [#7ba3d1]{url_value}[/#7ba3d1]"
                )
            else:
                lines.append(f"[#7ba3d1]{line}[/#7ba3d1]")

        return "\n".join(lines)
