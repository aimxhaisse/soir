"""VST plugin management CLI commands."""

from typing import Annotated

import typer
from rich.console import Console
from rich.table import Table
from soir._bindings import rt
from soir.cli.tui.engine_manager import EngineManager

vst_app = typer.Typer(help="VST plugin management.")

console = Console()

_TYPE_STYLE = {
    "effect": "[cyan]fx[/cyan]",
    "instrument": "[magenta]instrument[/magenta]",
}


@vst_app.command(name="ls")
def ls(
    plugin_type: Annotated[
        str | None,
        typer.Option(
            "--type",
            "-t",
            help="Filter by plugin type: 'effect' or 'instrument'.",
        ),
    ] = None,
) -> None:
    """List available VST3 plugins on this system."""
    if plugin_type is not None and plugin_type not in ("effect", "instrument"):
        typer.echo(
            f"Error: --type must be 'effect' or 'instrument', got '{plugin_type}'",
            err=True,
        )
        raise typer.Exit(1)

    engine_manager = EngineManager()
    try:
        success, message = engine_manager.initialize_standalone()

        if not success:
            typer.echo(f"Error: {message}", err=True)
            raise typer.Exit(1)

        plugins: list[dict[str, str]] = rt.vst_get_plugins_()
    finally:
        engine_manager.stop()

    visible = [
        p for p in plugins if plugin_type is None or p.get("type") == plugin_type
    ]

    table = Table(
        title="[bold]VST3 Plugins[/bold]",
        show_header=True,
        header_style="bold",
        border_style="bright_black",
    )
    table.add_column("Type")
    table.add_column("Name", style="bold white")
    table.add_column("Vendor", style="dim")
    table.add_column("Category", style="italic")

    if visible:
        for p in visible:
            type_label = _TYPE_STYLE.get(p.get("type", ""), p.get("type", ""))
            table.add_row(type_label, p["name"], p["vendor"], p["category"])
    else:
        table.add_row("", "[dim](none found)[/dim]", "", "")

    console.print(table)
