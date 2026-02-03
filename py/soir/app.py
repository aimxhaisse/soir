import typer

from soir.cli.samples import app as samples_app
from soir.cli.session import session_app
from soir.cli.www import www_app


app = typer.Typer()


app.add_typer(samples_app, name="samples")
app.add_typer(session_app, name="session")
app.add_typer(www_app, name="www")


@app.callback(invoke_without_command=True)
def main(ctx: typer.Context) -> None:
    if ctx.invoked_subcommand is None:
        typer.echo(ctx.get_help())


if __name__ == "__main__":
    app()
