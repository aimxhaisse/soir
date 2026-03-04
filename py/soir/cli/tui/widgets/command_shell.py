"""Command shell widget for Soir TUI.

This module provides an interactive command shell with input and output
display for executing soir-specific commands.
"""

from soir.cli.tui.commands import CommandInterpreter
from textual import events
from textual.app import ComposeResult
from textual.containers import Container
from textual.widgets import Input, RichLog


class HistoryInput(Input):
    """Input widget with shell-style command history navigation."""

    _history: list[str]
    _history_index: int

    def on_mount(self) -> None:
        self._history = []
        self._history_index = -1

    def add_to_history(self, command: str) -> None:
        if not self._history or self._history[-1] != command:
            self._history.append(command)
        self._history_index = -1

    def on_key(self, event: events.Key) -> None:
        if event.key == "up":
            if self._history and self._history_index < len(self._history) - 1:
                self._history_index += 1
                self.value = self._history[-(self._history_index + 1)]
                self.cursor_position = len(self.value)
            event.prevent_default()
            event.stop()
        elif event.key == "down":
            if self._history_index > 0:
                self._history_index -= 1
                self.value = self._history[-(self._history_index + 1)]
                self.cursor_position = len(self.value)
            elif self._history_index == 0:
                self._history_index = -1
                self.value = ""
                event.prevent_default()
                event.stop()


class CommandShellWidget(Container):
    """Command shell with input and output display."""

    def __init__(self) -> None:
        """Initialize the command shell widget."""
        super().__init__()
        self.interpreter: CommandInterpreter | None = None

    def compose(self) -> ComposeResult:
        """Compose the command shell layout.

        Yields:
            RichLog for output and Input for command entry
        """
        yield RichLog(id="command-output", highlight=True, markup=True, wrap=False)
        yield HistoryInput(
            id="command-input", placeholder="Enter command (type 'help')..."
        )

    def on_mount(self) -> None:
        """Focus the input field when mounted."""
        self.query_one("#command-input", HistoryInput).focus()

    def set_interpreter(self, interpreter: CommandInterpreter) -> None:
        """Set the command interpreter.

        Args:
            interpreter: CommandInterpreter instance to use
        """
        self.interpreter = interpreter

    def on_input_submitted(self, event: Input.Submitted) -> None:
        """Handle command submission.

        Args:
            event: Input submission event
        """
        if event.input.id != "command-input":
            return

        command = event.value.strip()
        if not command:
            return

        history_input = self.query_one("#command-input", HistoryInput)
        history_input.add_to_history(command)

        output = self.query_one("#command-output", RichLog)
        output.write(f"[#7ba3d1]> {command}[/#7ba3d1]")

        if self.interpreter:
            try:
                result = self.interpreter.execute(command)
                if result:
                    output.write(result)
            except Exception as e:  # noqa: BLE001
                output.write(f"[#c95757]Error: {e}[/#c95757]")
        else:
            output.write("[#c9a857]Command interpreter not initialized[/#c9a857]")

        event.input.value = ""

    def write_output(self, text: str) -> None:
        """Write output to the shell.

        Args:
            text: Text to write to output
        """
        output = self.query_one("#command-output", RichLog)
        output.write(text)
