"""Info panel widget for Soir TUI.

This module provides a panel that displays runtime information about
the Soir engine, including tracks, BPM, and output device.
"""

import math
from pathlib import Path
from typing import Any

from textual.reactive import reactive
from textual.widgets import Static


class InfoPanelWidget(Static):
    """Display panel for runtime information."""

    track_count = reactive(0)
    current_bpm = reactive(0.0)
    session_name = reactive("")
    level_left = reactive(0.0)
    level_right = reactive(0.0)

    def __init__(self, session_path: Path) -> None:
        """Initialize the info panel widget.

        Args:
            session_path: Path to the session directory
        """
        super().__init__()
        self.session_name = session_path.name

    def _render_level_bar(self, level: float, width: int = 20) -> str:
        """Render a single level meter bar.

        Args:
            level: Audio level (0.0 to 1.0+, can clip above 1.0)
            width: Width of the bar in characters

        Returns:
            Formatted string with colored level bar
        """
        if level <= 0.0:
            return f"[dim white]{'░' * width}[/dim white]"

        db = 20 * math.log10(max(level, 1e-10))
        db_normalized = (db + 60) / 60
        db_normalized = max(0.0, min(1.0, db_normalized))

        filled = int(db_normalized * width)
        filled = min(filled, width)

        green_threshold = int(width * 0.6)
        yellow_threshold = int(width * 0.85)

        bar = ""
        for i in range(width):
            if i < filled:
                if i < green_threshold:
                    bar += "[green]█[/green]"
                elif i < yellow_threshold:
                    bar += "[yellow]█[/yellow]"
                else:
                    bar += "[red]█[/red]"
            else:
                bar += "[dim white]░[/dim white]"

        return bar

    def render(self) -> str:
        """Render info panel content.

        Returns:
            Formatted string with runtime information
        """
        session_header = (
            f"[bold reverse cyan] Session {self.session_name} [/bold reverse cyan]"
        )

        left_bar = self._render_level_bar(self.level_left)
        right_bar = self._render_level_bar(self.level_right)

        lines = [
            session_header,
            "",
            f"[dim cyan]Tracks[/dim cyan]  [bold white]{self.track_count}[/bold white]",
            f"[dim cyan]BPM[/dim cyan]     [bold white]{self.current_bpm:.1f}[/bold white]",
            "",
            "[dim cyan]Output[/dim cyan]",
            f"  [dim]L[/dim] {left_bar}",
            f"  [dim]R[/dim] {right_bar}",
        ]

        return "\n".join(lines)

    def update_from_engine(self, info: dict[str, Any]) -> None:
        """Update display from engine runtime info.

        Args:
            info: Dictionary containing runtime information
        """
        self.track_count = len(info.get("tracks", []))
        self.current_bpm = info.get("bpm", 0.0)

        levels = info.get("levels")
        if levels:
            self.level_left = levels.peak_left
            self.level_right = levels.peak_right
