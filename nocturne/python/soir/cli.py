import typer

from soir.www.app import start_server
from soir.live.app import live_app


app = typer.Typer()

app.add_typer(live_app, name="live")


@app.command()
def www(
    port: int = typer.Option(5000, help="Port to run the server on"),
    dev: bool = typer.Option(False, help="Enable development mode"),
) -> None:
    """Start the Soir documentation website."""
    start_server(port=port, dev=dev)


@app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


if __name__ == "__main__":
    app()
