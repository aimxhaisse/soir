"""Command interpreter for Soir TUI.

This module provides a command interpreter for executing soir-specific
commands from the TUI command shell.
"""

from soir.cli.tui.engine_manager import EngineManager
from textual.app import App


class CommandInterpreter:
    """Interprets and executes soir-specific commands.

    The command interpreter provides a simple command-line interface
    within the TUI for controlling and inspecting the Soir engine.
    """

    COMMANDS = {
        "help": "Show available commands",
        "status": "Show engine status",
        "tracks": "List all tracks",
        "quit": "Quit the session",
    }

    def __init__(self, engine_manager: EngineManager, app: App):
        """Initialize the command interpreter.

        Args:
            engine_manager: Engine manager for accessing runtime info
            app: Textual app instance for control operations
        """
        self.engine = engine_manager
        self.app = app

    def execute(self, command: str) -> str:
        """Execute a command and return the result.

        Args:
            command: Command string to execute

        Returns:
            Formatted result string to display in the shell
        """
        parts = command.strip().split()
        if not parts:
            return ""

        cmd = parts[0].lower()
        args = parts[1:]

        if cmd == "help":
            return self._help()
        elif cmd == "status":
            return self._status()
        elif cmd == "tracks":
            return self._tracks()
        elif cmd == "quit":
            self.app.exit()
            return "Exiting..."
        else:
            return f"Unknown command: {cmd}. Type 'help' for available commands."

    def _help(self) -> str:
        """Return help text showing all available commands.

        Returns:
            Formatted help text
        """
        lines = ["Available commands:"]
        for cmd, desc in self.COMMANDS.items():
            lines.append(f"  {cmd:<20} {desc}")
        return "\n".join(lines)

    def _status(self) -> str:
        """Return engine status information.

        Returns:
            Formatted status information
        """
        running = self.engine.is_running()
        info = self.engine.get_runtime_info()
        return (
            f"Engine: {'Running' if running else 'Stopped'}\n"
            f"BPM: {info.get('bpm', 0):.1f}\n"
            f"Tracks: {len(info.get('tracks', []))}"
        )

    def _tracks(self) -> str:
        """List all configured tracks.

        Returns:
            Formatted track listing
        """
        info = self.engine.get_runtime_info()
        tracks = info.get("tracks", [])

        if not tracks:
            return "No tracks configured"

        lines = ["Tracks:"]
        for track in tracks:
            instrument = track.get("instrument", "Unknown")
            name = track.get("name", "Unknown")
            muted = " (muted)" if track.get("muted", False) else ""
            lines.append(f"  {name}: {instrument}{muted}")

        return "\n".join(lines)
