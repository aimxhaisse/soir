import typer
from pathlib import Path
import signal
from typing import Any
import threading

from soir.config import Config
from soir.watcher import Watcher

import soir._core as core
import soir._core.logging as logging


live_app = typer.Typer(help="Run and configure live session.")


@live_app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


@live_app.command()
def run(path: Path) -> None:
    """Run a Soir live session from the given path."""
    cfg_path = path / "etc" / "config.json"
    log_path = path / "var" / "log"

    logging.init(str(log_path), max_files=25)
    logging.info("Starting Soir live session")

    if not cfg_path.exists():
        logging.error(f"Config file not found at {cfg_path}")
        typer.echo(f"Error: Config file not found at {cfg_path}", err=True)
        raise typer.Exit(1)

    cfg = Config.load_from_path(str(cfg_path))
    soir = core.Soir()

    if not soir.init(str(cfg_path)):
        logging.error("Failed to initialize Soir")
        typer.echo("Error: Failed to initialize Soir", err=True)
        raise typer.Exit(1)

    if not soir.start():
        logging.error("Failed to start Soir")
        typer.echo("Error: Failed to start Soir", err=True)
        raise typer.Exit(1)

    watcher = Watcher(cfg, lambda code: soir.update_code(code))
    watcher.start()

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
