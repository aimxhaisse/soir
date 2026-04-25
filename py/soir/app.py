import typer

from soir.cli.samples import app as samples_app
from soir.cli.session import session_app
from soir.cli.vst import vst_app
from soir.cli.www import www_app
from soir.config import ensure_soir_home

app = typer.Typer(no_args_is_help=True)


app.add_typer(samples_app, name="samples")
app.add_typer(session_app, name="session")
app.add_typer(vst_app, name="vst")
app.add_typer(www_app, name="www")


@app.callback()
def main(ctx: typer.Context) -> None:
    ensure_soir_home()


if __name__ == "__main__":
    app()
