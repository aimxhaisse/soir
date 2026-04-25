import shutil
import signal
import threading
from pathlib import Path
from typing import Any

import typer
from soir import _resources
from soir.cast import CastServer
from soir.cli.tui.engine_manager import EngineManager

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
    from soir.cli.tui.engine_manager import redirect_python_io_to_tty

    # Redirect Python I/O to /dev/tty before Textual starts so that it captures
    # the tty fd instead of fd 1. When the engine later calls
    # logging.init(redirect_stdio=True), freopen() on fd 1 sends C++ output to
    # the log file without touching Textual's render target.
    redirect_python_io_to_tty()
    app = SoirTuiApp(path, verbose)
    app.run()


def _run_blocking(path: Path, verbose: bool) -> None:
    """Run session with blocking CLI interface (legacy mode)."""
    engine_manager = EngineManager(verbose)
    success, message = engine_manager.initialize_session(path)

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
def mk(name: Path) -> None:
    """Create a new session directory structure."""
    session_path = Path(name)
    session_name = session_path.name

    if not session_name or session_name.startswith("."):
        typer.echo("Error: Invalid session name.", err=True)
        raise typer.Exit(1)

    if session_path.exists():
        typer.echo(f"Error: Session '{session_name}' already exists.", err=True)
        raise typer.Exit(1)

    try:
        (session_path / "etc").mkdir(parents=True)
        (session_path / "lib" / "samples").mkdir(parents=True)
        (session_path / "var" / "log").mkdir(parents=True)

        shutil.copy(
            _resources.resources.config_path, session_path / "etc" / "config.json"
        )
        shutil.copy(_resources.resources.live_template_path, session_path / "live.py")

        typer.echo(f"Session '{session_name}' created at {session_path}")
    except (OSError, FileNotFoundError) as e:
        typer.echo(f"Error: {e}", err=True)
        raise typer.Exit(1)
