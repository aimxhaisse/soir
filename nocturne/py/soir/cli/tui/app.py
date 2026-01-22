"""Main TUI application for Soir.

This module provides the main Textual application that orchestrates
all widgets and manages the engine lifecycle.
"""

import time
from pathlib import Path

from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal, Vertical
from textual.reactive import reactive
from textual.widgets import Footer

from soir.cli.tui.commands import CommandInterpreter
from soir.cli.tui.engine_manager import EngineManager
from soir.cli.tui.log_tailer import LogTailer
from soir.cli.tui.widgets.command_shell import CommandShellWidget
from soir.cli.tui.widgets.header import HeaderWidget
from soir.cli.tui.widgets.info_panel import InfoPanelWidget
from soir.cli.tui.widgets.log_viewer import LogViewerWidget
from soir.rt import levels


class SoirTuiApp(App[None]):
    """Textual TUI for Soir live coding sessions."""

    CSS_PATH = "app.tcss"

    BINDINGS = [
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

    async def action_quit(self) -> None:
        """Override quit to ensure clean shutdown."""
        self._shutting_down = True
        if self.log_tailer:
            self.log_tailer.stop()
        self.engine_manager.stop()
        self.exit()

    def on_unmount(self) -> None:
        """Clean up when the app is closing."""
        if self.log_tailer:
            self.log_tailer.stop()
        self.engine_manager.stop()

    def _start_engine(self) -> None:
        """Worker to start the Soir engine (runs in thread)."""
        try:
            success, message = self.engine_manager.initialize()

            if success:
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

        except Exception as e:
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
            except Exception as e:
                self.call_from_thread(self.notify, f"Log tailing error: {e}", "warning")

    def _update_info_panel(self) -> None:
        """Worker to periodically update the info panel (runs in thread)."""
        info_panel = self.query_one(InfoPanelWidget)

        while not self._shutting_down:
            try:
                if self.engine_manager.is_running():
                    info = self.engine_manager.get_runtime_info()
                    info["levels"] = levels.get_master_levels()
                    self.call_from_thread(info_panel.update_from_engine, info)

                time.sleep(0.1)

            except Exception:
                time.sleep(0.5)

    def action_focus_logs(self) -> None:
        """Focus the log viewer for scrolling."""
        log_viewer = self.query_one(LogViewerWidget)
        log_viewer.focus()

    def action_focus_command(self) -> None:
        """Focus the command input."""
        command_shell = self.query_one(CommandShellWidget)
        command_input = command_shell.query_one("#command-input")
        command_input.focus()
