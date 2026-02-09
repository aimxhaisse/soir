import os
import os.path
from pathlib import Path
import signal
import typer
from typing import Any
import threading
import shutil

from soir.config import (
    Config,
    get_soir_dir,
)
from soir.watcher import Watcher

import soir._bindings as bindings
import soir._bindings.logging as logging

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
    os.chdir(str(path))

    cfg_path = "etc/config.json"
    log_path = "var/log"

    logging.init(log_path, max_files=25, verbose=verbose)
    logging.info("Starting Soir live session")

    if not os.path.exists(cfg_path):
        logging.error(f"Config file not found at {cfg_path}")
        typer.echo(f"Error: Config file not found at {cfg_path}", err=True)
        raise typer.Exit(1)

    cfg = Config.load_from_path(cfg_path)
    soir = bindings.Soir()

    if not soir.init(cfg_path):
        logging.error("Failed to initialize Soir")
        typer.echo("Error: Failed to initialize Soir", err=True)
        raise typer.Exit(1)

    if not soir.start():
        logging.error("Failed to start Soir")
        typer.echo("Error: Failed to start Soir", err=True)
        raise typer.Exit(1)
    logging.info("Soir started successfully")

    watcher = Watcher(cfg, lambda code: soir.update_code(code))
    watcher.start()
    logging.info("Watcher started successfully")

    shutdown_event = threading.Event()

    def signal_handler(signum: int, frame: Any) -> None:
        logging.info(f"Received signal {signum}, shutting down...")
        shutdown_event.set()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logging.info("Soir is running. Press Ctrl-C to stop.")
    shutdown_event.wait()

    watcher.stop()
    soir.stop()

    logging.shutdown()


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
