"""Command interpreter for Soir TUI.

This module provides a command interpreter for executing soir-specific
commands from the TUI command shell.
"""

import asyncio
from typing import Any

from soir.cli.tui.engine_manager import EngineManager
from soir.rt.sampler import packs as get_packs, samples as get_samples
from textual.app import App


class CommandInterpreter:
    """Interprets and executes soir-specific commands.

    The command interpreter provides a simple command-line interface
    within the TUI for controlling and inspecting the Soir engine.
    """

    COMMANDS = {
        "help": "show available commands",
        "status": "show engine status",
        "tracks": "list tracks",
        "packs": "list available sample packs",
        "quit": "quit the session",
    }

    def __init__(self, engine_manager: EngineManager, app: App[Any]):
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

        if cmd == "help":
            return self._help()
        elif cmd == "status":
            return self._status()
        elif cmd == "tracks":
            return self._tracks()
        elif cmd == "packs":
            return self._packs()
        elif cmd == "quit":
            asyncio.create_task(self.app.action_quit())
            return "exiting..."
        else:
            return f"unknown command: {cmd}. Type 'help' for available commands."

    def _help(self) -> str:
        """Return help text showing all available commands.

        Returns:
            Formatted help text
        """
        lines = []
        for cmd, desc in self.COMMANDS.items():
            lines.append(f"{cmd:<20} {desc}")
        return "\n".join(lines)

    def _status(self) -> str:
        """Return engine status information.

        Returns:
            Formatted status information
        """
        running = self.engine.is_running()
        info = self.engine.get_runtime_info()
        return (
            f"engine: {'Running' if running else 'Stopped'}\n"
            f"bpm: {info.get('bpm', 0):.1f}\n"
            f"beat: {int(info.get('beat', 0))}\n"
            f"tracks: {len(info.get('tracks', []))}"
        )

    def _tracks(self) -> str:
        """List all configured tracks.

        Returns:
            Formatted track listing
        """
        info = self.engine.get_runtime_info()
        tracks: list[dict[str, Any]] = info.get("tracks", [])

        if not tracks:
            return "no tracks configured"

        lines = []
        for track in tracks:
            instrument = track.get("instrument", "unknown")
            name = track.get("name", "unknown")
            muted = " (muted)" if track.get("muted", False) else ""
            lines.append(f"{name}: {instrument}{muted}")

        return "\n".join(lines)

    def _packs(self) -> str:
        """List available sample packs.

        Returns:
            Formatted pack listing
        """
        packs = get_packs()

        if not packs:
            return "no sample packs loaded"

        lines = []
        for pack in sorted(packs):
            samples = get_samples(pack)
            lines.append(f"{pack}: {len(samples)} samples")
        return "\n".join(lines)
