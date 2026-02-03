"""Command interpreter for Soir TUI.

This module provides a command interpreter for executing soir-specific
commands from the TUI command shell.
"""

import asyncio
from typing import Any

from soir.cli.tui.engine_manager import EngineManager
from soir._bindings.rt import (
    get_audio_in_devices_,
    get_audio_out_devices_,
    get_midi_out_devices_,
    start_recording_,
    stop_recording_,
    vst_get_plugins_,
)
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
        "audio-out": "list audio output devices",
        "audio-in": "list audio input devices",
        "midi-out": "list MIDI output devices",
        "vst list": "list available and instantiated VST plugins",
        "record start <file.wav>": "start recording to file",
        "record stop": "stop recording",
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
        self._recording_file: str | None = None

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
        elif cmd == "audio-out":
            return self._audio_out()
        elif cmd == "audio-in":
            return self._audio_in()
        elif cmd == "midi-out":
            return self._midi_out()
        elif cmd == "record":
            return self._record(parts[1:])
        elif cmd == "vst":
            return self._vst(parts[1:])
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
            lines.append(f"{cmd:<26} {desc}")
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

    def _audio_out(self) -> str:
        """List audio output devices.

        Returns:
            Formatted device listing
        """
        devices = get_audio_out_devices_()
        if not devices:
            return "no audio output devices found"
        lines = []
        for d in devices:
            default_marker = " *" if d.get("is_default", False) else ""
            lines.append(
                f"{d['id']}: {d['name']}{default_marker} ({d.get('channels', '?')}ch)"
            )
        return "\n".join(lines)

    def _audio_in(self) -> str:
        """List audio input devices.

        Returns:
            Formatted device listing
        """
        devices = get_audio_in_devices_()
        if not devices:
            return "no audio input devices found"
        lines = []
        for d in devices:
            default_marker = " *" if d.get("is_default", False) else ""
            lines.append(
                f"{d['id']}: {d['name']}{default_marker} ({d.get('channels', '?')}ch)"
            )
        return "\n".join(lines)

    def _midi_out(self) -> str:
        """List MIDI output devices.

        Returns:
            Formatted device listing
        """
        devices = get_midi_out_devices_()
        if not devices:
            return "no MIDI output devices found"
        lines = []
        for idx, name in devices:
            lines.append(f"{idx}: {name}")
        return "\n".join(lines)

    def _record(self, args: list[str]) -> str:
        """Handle record commands.

        Args:
            args: Command arguments (start/stop and optional file path)

        Returns:
            Result message
        """
        if not args:
            if self._recording_file:
                return f"recording to: {self._recording_file}"
            return "usage: record start <file.wav> | record stop"

        subcmd = args[0].lower()

        if subcmd == "start":
            if len(args) < 2:
                return "usage: record start <file.wav>"
            file_path = args[1]
            if self._recording_file:
                return f"already recording to: {self._recording_file}"
            if start_recording_(file_path):
                self._recording_file = file_path
                return f"recording started: {file_path}"
            return "failed to start recording"

        elif subcmd == "stop":
            return self.stop_recording()

        return "usage: record start <file.wav> | record stop"

    def stop_recording(self) -> str:
        """Stop any active recording.

        Returns:
            Result message
        """
        if not self._recording_file:
            return "not recording"
        if stop_recording_():
            file_path = self._recording_file
            self._recording_file = None
            return f"recording stopped: {file_path}"
        return "failed to stop recording"

    def _vst(self, args: list[str]) -> str:
        """Handle VST commands.

        Args:
            args: Command arguments

        Returns:
            Formatted VST information
        """
        if not args or args[0].lower() != "list":
            return "usage: vst list"

        lines: list[str] = []

        plugins = vst_get_plugins_()
        lines.append(f"[Available VST Plugins: {len(plugins)}]")
        if plugins:
            for p in plugins:
                lines.append(f"  {p['name']} ({p['vendor']})")
        else:
            lines.append("  (none found)")

        lines.append("")

        info = self.engine.get_runtime_info()
        tracks: list[dict[str, Any]] = info.get("tracks", [])
        instantiated: list[str] = []
        for track in tracks:
            track_name = track.get("name", "unknown")
            fxs = track.get("fxs", [])
            for fx in fxs:
                if fx.get("type") == "vst":
                    extra = fx.get("extra", {})
                    plugin_name = extra.get("plugin", "unknown")
                    fx_name = fx.get("name", "unnamed")
                    instantiated.append(f"  {track_name}/{fx_name}: {plugin_name}")

        lines.append(f"[Instantiated VST Effects: {len(instantiated)}]")
        if instantiated:
            lines.extend(instantiated)
        else:
            lines.append("  (none)")

        return "\n".join(lines)
