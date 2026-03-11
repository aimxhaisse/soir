"""Audio device widget for Soir TUI."""

from typing import Any, cast

from textual.reactive import reactive
from textual.widgets import Static


class AudioDeviceWidget(Static):
    """Clickable widget showing current audio output device."""

    current_device: reactive[str] = reactive("default")

    def render(self) -> str:
        if self.current_device == "none":
            label = "disabled"
        elif self.current_device == "default":
            label = "system default"
        else:
            label = self.current_device
        return (
            f"[#4a5878]audio out[/#4a5878]  "
            f"[#b8c0cc]{label}[/#b8c0cc]  "
            f"[#4a5878]▸[/#4a5878]"
        )

    def on_click(self) -> None:
        cast(Any, self.app).action_pick_audio_out()
