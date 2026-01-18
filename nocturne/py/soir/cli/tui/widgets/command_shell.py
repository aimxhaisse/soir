"""Command shell widget for Soir TUI.

This module provides an interactive command shell with input and output
display for executing soir-specific commands.
"""

from textual.app import ComposeResult
from textual.containers import Container
from textual.widgets import Input, RichLog

from soir.cli.tui.commands import CommandInterpreter


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
        yield Input(id="command-input", placeholder="Enter command (type 'help')...")

    def on_mount(self) -> None:
        """Focus the input field when mounted."""
        self.query_one("#command-input", Input).focus()

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

        output = self.query_one("#command-output", RichLog)
        output.write(f"[bold cyan]> {command}[/bold cyan]")

        if self.interpreter:
            try:
                result = self.interpreter.execute(command)
                if result:
                    output.write(result)
            except Exception as e:
                output.write(f"[red]Error: {e}[/red]")
        else:
            output.write("[yellow]Command interpreter not initialized[/yellow]")

        event.input.value = ""

    def write_output(self, text: str) -> None:
        """Write output to the shell.

        Args:
            text: Text to write to output
        """
        output = self.query_one("#command-output", RichLog)
        output.write(text)
