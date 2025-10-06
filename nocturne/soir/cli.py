import typer

from soir import hello_world

app = typer.Typer()


@app.command()
def greet() -> None:
    """Greet from C++."""
    print(hello_world())


def main() -> None:
    app()


if __name__ == "__main__":
    main()
