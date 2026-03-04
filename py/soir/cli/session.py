import shutil
import signal
import threading
from pathlib import Path
from typing import Any

import typer
from soir.cast import CastServer
from soir.cli.tui.engine_manager import EngineManager
from soir.config import get_soir_dir

session_app = typer.Typer(help="Create and manage Soir sessions.")


@session_app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


@session_app.command()
def run(path: Path, verbose: bool = False, no_tui: bool = False) -> None:
    """Run a Soir session from the given path."""
    if not path.is_dir():
        typer.echo(f"Error: The path '{path}' is not a valid directory.", err=True)
        raise typer.Exit(1)

    if no_tui:
        _run_blocking(path, verbose)
    else:
        _run_tui(path, verbose)


def _run_tui(path: Path, verbose: bool) -> None:
    """Run session with TUI interface."""
    from soir.cli.tui import SoirTuiApp

    app = SoirTuiApp(path, verbose)
    app.run()


def _run_blocking(path: Path, verbose: bool) -> None:
    """Run session with blocking CLI interface (legacy mode)."""
    engine_manager = EngineManager(path, verbose)
    success, message = engine_manager.initialize()

    if not success:
        typer.echo(f"Error: {message}", err=True)
        raise typer.Exit(1)

    cast_server: CastServer | None = None
    if engine_manager.config and engine_manager.config.cast.enabled:
        cast_server = CastServer(engine_manager.config)
        cast_server.start()

    shutdown_event = threading.Event()

    def signal_handler(signum: int, frame: Any) -> None:
        shutdown_event.set()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    shutdown_event.wait()

    if cast_server:
        cast_server.stop()
    engine_manager.stop()


@session_app.command()
def mk(name: str) -> None:
    """Create a new session directory structure."""
    if not name or "/" in name or "\\" in name or name.startswith("."):
        typer.echo("Error: Invalid session name.", err=True)
        raise typer.Exit(1)

    session_path = Path(name)

    if session_path.exists():
        typer.echo(f"Error: Session '{name}' already exists.", err=True)
        raise typer.Exit(1)

    try:
        (session_path / "etc").mkdir(parents=True)
        (session_path / "lib" / "samples").mkdir(parents=True)
        (session_path / "var" / "log").mkdir(parents=True)

        soir_dir = get_soir_dir()
        config_template = Path(soir_dir) / "etc" / "config.default.json"
        live_template = Path(soir_dir) / "etc" / "live.default.py"

        shutil.copy(config_template, session_path / "etc" / "config.json")
        shutil.copy(live_template, session_path / "live.py")

        typer.echo(f"Session '{name}' created at {session_path}")
    except (OSError, FileNotFoundError) as e:
        typer.echo(f"Error: {e}", err=True)
        raise typer.Exit(1)
