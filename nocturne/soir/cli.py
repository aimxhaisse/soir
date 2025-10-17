import typer
from pathlib import Path

from soir import hello_world
from soir.www.app import start_server
import soir._core as core

app = typer.Typer()
session_app = typer.Typer()
app.add_typer(session_app, name="session")


@app.command()
def greet() -> None:
    """Greet from C++."""
    print(hello_world())


@app.command()
def www(
    port: int = typer.Option(5000, help="Port to run the server on"),
    dev: bool = typer.Option(False, help="Enable development mode"),
) -> None:
    """Start the Soir documentation website."""
    start_server(port=port, dev=dev)


@session_app.command()
def run(path: Path) -> None:
    """Run a Soir session from the given path."""
    config_path = path / "etc" / "config.yaml"

    if not config_path.exists():
        typer.echo(f"Error: Config file not found at {config_path}", err=True)
        raise typer.Exit(1)

    success = core.Start(str(config_path))
    if not success:
        typer.echo("Error: Failed to start Soir", err=True)
        raise typer.Exit(1)


def main() -> None:
    app()


if __name__ == "__main__":
    main()
