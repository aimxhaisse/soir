import typer
from pathlib import Path

from soir.www.app import start_server
import soir._core as core

app = typer.Typer()
live_app = typer.Typer(help="Run and configure live session.")
app.add_typer(live_app, name="live")


@app.command()
def www(
    port: int = typer.Option(5000, help="Port to run the server on"),
    dev: bool = typer.Option(False, help="Enable development mode"),
) -> None:
    """Start the Soir documentation website."""
    start_server(port=port, dev=dev)


@live_app.command()
def run(path: Path) -> None:
    """Run a Soir live session from the given path."""
    config_path = path / "etc" / "config.json"

    if not config_path.exists():
        typer.echo(f"Error: Config file not found at {config_path}", err=True)
        raise typer.Exit(1)

    success = core.Start(str(config_path))
    if not success:
        typer.echo("Error: Failed to start Soir", err=True)
        raise typer.Exit(1)


@app.callback(invoke_without_command=True)
def main(ctx: typer.Context):
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


if __name__ == "__main__":
    app()
