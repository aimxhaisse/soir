"""Info panel widget for Soir TUI.

This module provides a panel that displays runtime information about
the Soir engine, including tracks, BPM, and output device.
"""

from pathlib import Path

from textual.reactive import reactive
from textual.widgets import Static


class InfoPanelWidget(Static):
    """Display panel for runtime information."""

    track_count: int = reactive(0)
    current_bpm: float = reactive(0.0)
    session_name: str = reactive("")

    def __init__(self, session_path: Path) -> None:
        """Initialize the info panel widget.

        Args:
            session_path: Path to the session directory
        """
        super().__init__()
        self.session_name = session_path.name

    def render(self) -> str:
        """Render info panel content.

        Returns:
            Formatted string with runtime information
        """
        session_header = f"[bold reverse cyan] Session {self.session_name} [/bold reverse cyan]"

        lines = [
            session_header,
            "",
            f"[dim cyan]Tracks[/dim cyan]  [bold white]{self.track_count}[/bold white]",
            f"[dim cyan]BPM[/dim cyan]     [bold white]{self.current_bpm:.1f}[/bold white]",
        ]

        return "\n".join(lines)

    def update_from_engine(self, info: dict) -> None:
        """Update display from engine runtime info.

        Args:
            info: Dictionary containing runtime information
        """
        self.track_count = len(info.get("tracks", []))
        self.current_bpm = info.get("bpm", 0.0)
