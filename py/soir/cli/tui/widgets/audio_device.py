"""Audio device widget for Soir TUI."""

from textual.reactive import reactive
from textual.widgets import Static


class AudioDeviceWidget(Static):
    """Clickable widget showing current audio output device."""

    current_device: reactive[str] = reactive("")

    def render(self) -> str:
        label = self.current_device if self.current_device else "system default"
        return (
            f"[#4a5878]audio out[/#4a5878]  "
            f"[#b8c0cc]{label}[/#b8c0cc]  "
            f"[#4a5878]▸[/#4a5878]"
        )

    def on_click(self) -> None:
        self.app.action_pick_audio_out()
