import typer
from pathlib import Path

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
    watcher = Watcher(cfg, lambda code: core.update_code(code))
    soir = core.Soir()

    if not soir.init(str(cfg_path)):
        logging.error("Failed to initialize Soir")
        typer.echo("Error: Failed to initialize Soir", err=True)
        raise typer.Exit(1)

    if not soir.start():
        logging.error("Failed to start Soir")
        typer.echo("Error: Failed to start Soir", err=True)
        raise typer.Exit(1)

    watcher.start()

    # Here we need to find a way to wait for ctrl-c or termination signal

    watcher.stop()
    soir.stop()

    logging.shutdown()

