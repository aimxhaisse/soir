import typer

from soir.www.app import start_server
from soir.cli.live import live_app
from soir.cli.www import www_app


app = typer.Typer()


app.add_typer(live_app, name="live")
app.add_typer(www_app, name="www")


@app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


if __name__ == "__main__":
    app()
