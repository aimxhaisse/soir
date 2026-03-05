"""Main TUI application for Soir.

This module provides the main Textual application that orchestrates
all widgets and manages the engine lifecycle.
"""

import json
import time
from pathlib import Path
from typing import ClassVar

from soir import _bindings
from soir._bindings.rt import get_audio_out_devices_, pump_ui_events_
from soir.cast import CastServer
from soir.cli.tui.commands import CommandInterpreter
from soir.cli.tui.engine_manager import EngineManager
from soir.cli.tui.log_tailer import LogTailer
from soir.cli.tui.widgets.audio_device import AudioDeviceWidget
from soir.cli.tui.widgets.command_shell import CommandShellWidget
from soir.cli.tui.widgets.header import HeaderWidget
from soir.cli.tui.widgets.info_panel import InfoPanelWidget
from soir.cli.tui.widgets.log_viewer import LogViewerWidget
from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Container, Horizontal, Vertical
from textual.reactive import reactive
from textual.screen import ModalScreen
from textual.widgets import Footer, ListItem, ListView


class SoirTuiApp(App[None]):
    """Textual TUI for Soir live coding sessions."""

    CSS_PATH = "app.tcss"

    BINDINGS: ClassVar[list[Binding | tuple[str, str] | tuple[str, str, str]]] = [
        Binding("ctrl+c", "quit", "Quit"),
        Binding("ctrl+l", "focus_logs", "Focus Logs"),
        Binding("ctrl+k", "focus_command", "Focus Command"),
    ]

    engine_status = reactive("stopped")
    track_count = reactive(0)
    current_bpm = reactive(0.0)

    def __init__(self, session_path: Path, verbose: bool = False):
        """Initialize the TUI application.

        Args:
            session_path: Path to the session directory
            verbose: Enable verbose logging
        """
        super().__init__()
        self.session_path = session_path.resolve()
        self.verbose = verbose
        self.engine_manager = EngineManager(self.session_path, verbose)
        self.log_tailer: LogTailer | None = None
        self.command_interpreter: CommandInterpreter | None = None
        self.cast_server: CastServer | None = None
        self._shutting_down = False

    def compose(self) -> ComposeResult:
        """Build the UI layout.

        Yields:
            All widgets that make up the interface
        """
        yield HeaderWidget()
        with Horizontal(id="main-container"):
            with Vertical(id="left-pane"):
                yield CommandShellWidget()
            with Vertical(id="right-pane"):
                yield InfoPanelWidget(self.session_path)
                yield AudioDeviceWidget()
        yield LogViewerWidget()
        yield Footer()

    def on_mount(self) -> None:
        """Initialize engine and start background tasks."""
        self.command_interpreter = CommandInterpreter(self.engine_manager, self)

        command_shell = self.query_one(CommandShellWidget)
        command_shell.set_interpreter(self.command_interpreter)
        command_shell.write_output(
            "[#4a5878]No path but this\nBell crickets singing\nIn the moonlight[/#4a5878]"
        )

        log_viewer = self.query_one(LogViewerWidget)
        log_dir = self.session_path / "var" / "log"
        self.log_tailer = LogTailer(
            log_dir, lambda line: self.call_from_thread(log_viewer.add_log_line, line)
        )

        self.run_worker(self._start_engine, thread=True, exclusive=True)
        self.run_worker(self._tail_logs, thread=True, exclusive=True)
        self.run_worker(self._update_info_panel, thread=True, exclusive=True)
        self.set_interval(1 / 60, self._pump_ui_events)

    async def action_quit(self) -> None:
        """Override quit to ensure clean shutdown."""
        self._shutting_down = True
        if self.command_interpreter:
            self.command_interpreter.stop_recording()
        if self.log_tailer:
            self.log_tailer.stop()
        if self.cast_server:
            self.cast_server.stop()
        self.engine_manager.stop()
        self.exit()

    def on_unmount(self) -> None:
        """Clean up when the app is closing."""
        if self.command_interpreter:
            self.command_interpreter.stop_recording()
        if self.log_tailer:
            self.log_tailer.stop()
        if self.cast_server:
            self.cast_server.stop()
        self.engine_manager.stop()

    def _start_engine(self) -> None:
        """Worker to start the Soir engine (runs in thread)."""
        try:
            success, message = self.engine_manager.initialize()

            if success:
                cfg = self.engine_manager.config
                if cfg and cfg.cast.enabled:
                    self.cast_server = CastServer(cfg)
                    self.cast_server.start()

                initial_device = cfg.dsp.audio_output_device if cfg else ""
                device_widget = self.query_one(AudioDeviceWidget)
                self.call_from_thread(
                    setattr, device_widget, "current_device", initial_device
                )

                self.call_from_thread(setattr, self, "engine_status", "running")
                self.call_from_thread(
                    self.notify, "Engine started successfully", "information"
                )

                command_shell = self.query_one(CommandShellWidget)
                self.call_from_thread(
                    command_shell.write_output,
                    "[#5b9a6d]Engine started successfully[/#5b9a6d]",
                )

            else:
                self.call_from_thread(setattr, self, "engine_status", "error")
                self.call_from_thread(
                    self.notify, f"Failed to start engine: {message}", "error"
                )

                command_shell = self.query_one(CommandShellWidget)
                self.call_from_thread(
                    command_shell.write_output, f"[#c95757]Error: {message}[/#c95757]"
                )

        except Exception as e:  # noqa: BLE001
            self.call_from_thread(setattr, self, "engine_status", "error")
            self.call_from_thread(self.notify, f"Unexpected error: {e}", "error")

            command_shell = self.query_one(CommandShellWidget)
            self.call_from_thread(
                command_shell.write_output, f"[#c95757]Unexpected error: {e}[/#c95757]"
            )

    def _tail_logs(self) -> None:
        """Worker to tail log files (runs in thread)."""
        if self.log_tailer:
            try:
                self.log_tailer.start()
            except Exception as e:  # noqa: BLE001
                self.call_from_thread(self.notify, f"Log tailing error: {e}", "warning")

    def _update_info_panel(self) -> None:
        """Worker to periodically update the info panel (runs in thread)."""
        info_panel = self.query_one(InfoPanelWidget)

        while not self._shutting_down:
            try:
                if self.engine_manager.is_running():
                    info = json.loads(_bindings.state.get_snapshot_())
                    self.call_from_thread(info_panel.update_from_engine, info)

                time.sleep(0.1)

            except Exception:  # noqa: BLE001 - info panel update loop must not crash
                time.sleep(0.5)

    def watch_engine_status(self, status: str) -> None:
        """React to engine status changes on the event loop."""
        if status != "running":
            return
        cfg = self.engine_manager.config
        if cfg and cfg.cast.enabled:
            self.query_one(HeaderWidget).cast_url = f"http://localhost:{cfg.cast.port}"
        elif cfg and cfg.dsp.enable_streaming:
            url = f"http://localhost:{cfg.dsp.streaming_port}/stream.opus"
            self.query_one(HeaderWidget).streaming_url = url

    def _pump_ui_events(self) -> None:
        """Pump platform UI events so native windows (e.g. VST editors) render."""
        try:
            pump_ui_events_()
        except Exception:  # noqa: BLE001, S110 - silently ignore VST UI pump errors
            pass

    def action_focus_logs(self) -> None:
        """Focus the log viewer for scrolling."""
        log_viewer = self.query_one(LogViewerWidget)
        log_viewer.focus()

    def action_focus_command(self) -> None:
        """Focus the command input."""
        command_shell = self.query_one(CommandShellWidget)
        command_input = command_shell.query_one("#command-input")
        command_input.focus()

    def action_pick_audio_out(self) -> None:
        """Open audio output device picker modal."""
        devices = get_audio_out_devices_()
        if not devices:
            self.notify("No audio output devices found", severity="warning")
            return

        def on_picked(device_name: str | None) -> None:
            if device_name is None:
                return

            def do_reload() -> None:
                success, message = self.engine_manager.set_audio_output_device(
                    device_name
                )
                color = "#5b9a6d" if success else "#c95757"
                severity = "information" if success else "error"
                self.call_from_thread(self.notify, message, severity=severity)
                shell = self.query_one(CommandShellWidget)
                self.call_from_thread(
                    shell.write_output, f"[{color}]{message}[/{color}]"
                )
                if success:
                    widget = self.query_one(AudioDeviceWidget)
                    self.call_from_thread(
                        setattr, widget, "current_device", device_name
                    )

            self.run_worker(do_reload, thread=True, name="audio-reload")

        self.push_screen(AudioOutPickerScreen(devices), on_picked)


class AudioOutPickerScreen(ModalScreen[str | None]):
    """Modal screen for picking an audio output device."""

    CSS = """
    AudioOutPickerScreen {
        align: center middle;
    }

    #picker-container {
        width: 60;
        height: auto;
        max-height: 20;
        border: solid #2a3040;
        background: #12161f;
        padding: 1 2;
    }

    #picker-title {
        color: #b8c0cc;
        text-align: center;
        height: 1;
        margin-bottom: 1;
    }

    ListView {
        background: #12161f;
        border: none;
    }

    ListItem {
        background: #12161f;
        color: #b8c0cc;
        padding: 0 1;
    }

    ListItem:hover {
        background: #1a2030;
    }

    ListView > ListItem.--highlight {
        background: #2a3040;
        color: #7ba3d1;
    }
    """

    def __init__(self, devices: list[dict]) -> None:
        super().__init__()
        self._devices = devices

    def compose(self) -> ComposeResult:
        with Container(id="picker-container"):
            from textual.widgets import Label

            yield Label("Select Audio Output Device", id="picker-title")
            items = [ListItem(Label("(system default)"))]
            for d in self._devices:
                marker = " *" if d.get("is_default", False) else ""
                items.append(ListItem(Label(f"{d['name']}{marker}")))
            yield ListView(*items)

    def on_key(self, event: object) -> None:
        from textual.events import Key

        if isinstance(event, Key) and event.key == "escape":
            self.dismiss(None)

    def on_list_view_selected(self, event: ListView.Selected) -> None:
        idx = event.list_view.index
        if idx is None:
            self.dismiss(None)
            return
        if idx == 0:
            self.dismiss("")
        else:
            device = self._devices[idx - 1]
            self.dismiss(device["name"])
