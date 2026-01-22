"""Info panel widget for Soir TUI.

This module provides a panel that displays runtime information about
the Soir engine, including tracks, BPM, and output device.
"""

import math
from pathlib import Path
from typing import Any

from textual.reactive import reactive
from textual.widgets import Static


# Color palette matching website CSS
_COLOR_TEXT = "#b8c0cc"
_COLOR_TEXT_DIM = "#4a5878"
_COLOR_ACCENT = "#7ba3d1"
_COLOR_PURPLE = "#9a8bd9"


class InfoPanelWidget(Static):
    """Display panel for runtime information."""

    track_count = reactive(0)
    current_bpm = reactive(0.0)
    session_name = reactive("")
    level_left = reactive(0.0)
    level_right = reactive(0.0)
    peak_left = reactive(0.0)
    peak_right = reactive(0.0)

    def __init__(self, session_path: Path) -> None:
        """Initialize the info panel widget.

        Args:
            session_path: Path to the session directory
        """
        super().__init__()
        self.session_name = session_path.name
        self._peak_hold_left = 0.0
        self._peak_hold_right = 0.0
        self._peak_decay = 0.92

    def _level_to_db(self, level: float) -> float:
        """Convert linear level to dB.

        Args:
            level: Linear audio level

        Returns:
            Level in dB, clamped to -60
        """
        if level <= 0.0:
            return -60.0
        return max(-60.0, 20 * math.log10(level))

    def _render_meter(self, level: float, peak: float, width: int = 24) -> str:
        """Render a refined level meter with peak hold.

        Args:
            level: Current audio level (linear)
            peak: Peak hold level (linear)
            width: Width of the meter in characters

        Returns:
            Formatted string with level meter
        """
        db = self._level_to_db(level)
        peak_db = self._level_to_db(peak)

        # Map -60dB to 0dB onto the meter width
        db_norm = (db + 60) / 60
        peak_norm = (peak_db + 60) / 60

        db_norm = max(0.0, min(1.0, db_norm))
        peak_norm = max(0.0, min(1.0, peak_norm))

        filled = int(db_norm * width)
        peak_pos = int(peak_norm * width)

        # Thresholds for color zones
        green_end = int(width * 0.65)  # -60 to -21 dB
        yellow_end = int(width * 0.85)  # -21 to -9 dB
        # Rest is red                   # -9 to 0 dB

        bar = ""
        for i in range(width):
            is_peak = i == peak_pos - 1 and peak_pos > 0

            if i < filled or is_peak:
                # Determine color based on position
                if i < green_end:
                    color = "#5b9a6d"  # Muted green
                elif i < yellow_end:
                    color = "#c9a857"  # Muted yellow
                else:
                    color = "#c95757"  # Muted red

                if is_peak and i >= filled:
                    bar += f"[{color}]▌[/{color}]"
                else:
                    bar += f"[{color}]▌[/{color}]"
            else:
                bar += f"[{_COLOR_TEXT_DIM}]▪[/{_COLOR_TEXT_DIM}]"

        return bar

    def _format_db(self, level: float) -> str:
        """Format level as dB string.

        Args:
            level: Linear audio level

        Returns:
            Formatted dB string
        """
        db = self._level_to_db(level)
        if db <= -60:
            return " -∞ "
        return f"{db:+.0f}".rjust(4)

    def render(self) -> str:
        """Render info panel content.

        Returns:
            Formatted string with runtime information
        """
        # Session header with subtle styling
        header = f"[{_COLOR_ACCENT}]─── {self.session_name} ───[/{_COLOR_ACCENT}]"

        # Stats section
        stats = [
            "",
            f"[{_COLOR_TEXT_DIM}]tracks[/{_COLOR_TEXT_DIM}]  "
            f"[{_COLOR_TEXT}]{self.track_count}[/{_COLOR_TEXT}]",
            f"[{_COLOR_TEXT_DIM}]bpm[/{_COLOR_TEXT_DIM}]     "
            f"[{_COLOR_TEXT}]{self.current_bpm:.1f}[/{_COLOR_TEXT}]",
        ]

        # Level meters with dB readout
        left_meter = self._render_meter(self.level_left, self.peak_left)
        right_meter = self._render_meter(self.level_right, self.peak_right)

        left_db = self._format_db(self.level_left)
        right_db = self._format_db(self.level_right)

        meters = [
            "",
            f"[{_COLOR_TEXT_DIM}]output[/{_COLOR_TEXT_DIM}]",
            "",
            f"[{_COLOR_TEXT_DIM}]L[/{_COLOR_TEXT_DIM}] {left_meter} "
            f"[{_COLOR_TEXT_DIM}]{left_db}[/{_COLOR_TEXT_DIM}]",
            f"[{_COLOR_TEXT_DIM}]R[/{_COLOR_TEXT_DIM}] {right_meter} "
            f"[{_COLOR_TEXT_DIM}]{right_db}[/{_COLOR_TEXT_DIM}]",
        ]

        # Scale markers
        scale = f"[{_COLOR_TEXT_DIM}]  -60        -20    -9  0[/{_COLOR_TEXT_DIM}]"
        meters.append(scale)

        return "\n".join([header] + stats + meters)

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

            # Update peak hold with decay
            if levels.peak_left > self._peak_hold_left:
                self._peak_hold_left = levels.peak_left
            else:
                self._peak_hold_left *= self._peak_decay

            if levels.peak_right > self._peak_hold_right:
                self._peak_hold_right = levels.peak_right
            else:
                self._peak_hold_right *= self._peak_decay

            self.peak_left = self._peak_hold_left
            self.peak_right = self._peak_hold_right
