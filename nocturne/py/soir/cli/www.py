import typer

from soir.www.app import start_server


www_app = typer.Typer(help="Run web sever.")


@www_app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


@www_app.command()
def run(
    port: int = typer.Option(5000, help="Port to run the server on"),
    dev: bool = typer.Option(False, help="Enable development mode"),
) -> None:
    """Start the Soir documentation website."""
    start_server(port=port, dev=dev)
