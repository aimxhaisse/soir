import typer

from soir import hello_world
from soir.www.app import start_server

app = typer.Typer()


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


def main() -> None:
    app()


if __name__ == "__main__":
    main()
