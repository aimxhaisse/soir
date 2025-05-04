"""
Session management module for Soir.

This module provides utilities for creating and managing Soir sessions.
"""

import typer
from typing import Optional

from textual import events
from textual.app import (
    App,
    ComposeResult,
    RenderResult,
)
from textual.containers import (
    Container,
    Horizontal,
)
from textual.reactive import reactive
from textual.widgets import (
    Button,
    Header,
    Label,
    Markdown,
    Select,
    Static,
    TabbedContent,
    TabPane,
)
from textual.widget import Widget

from soir.system import get_audio_devices

HAIKU = """
Code flows like music
Algorithms dance through sound
Python sings alive
"""


class SessionApp(App):
    """Control Center for Soir."""

    CSS = """
    Screen {
        padding: 2;
    }

    #haiku {
        text-align: left;
        align: center middle;
    }

    #run-button-container {
       align: right top;
       margin: 1 0;
    }
    """

    def __init__(self, name: str):
        super().__init__()

        self.title = "soir"
        self.sub_title = " https://soir.dev"
        self.theme = "nord"

        self.load()

    def load(self):
        """Load configuration's file and set up the app."""
        pass

    def compose(self) -> ComposeResult:
        """Create child widgets for the app."""

        yield Header(show_clock=True)

        with TabbedContent(initial="overview"):

            with TabPane("Overview", id="overview"):
                yield Static(HAIKU, id="haiku")
                with Container(id="run-button-container"):
                    yield Button("Run", variant="primary", id="run-button")

            with TabPane("Audio", id="audio"):
                pass


app = typer.Typer(
    help="Session management commands",
    no_args_is_help=True
)


@app.command("init")
def init_session(
    name: str = typer.Option(..., "--name", "-n", help="Session name"),
):
    """Initialize a new Soir session."""
    session_app = SessionApp(name)
    session_app.run()


@app.command("run")
def run_session(
    name: str = typer.Option(..., "--name", "-n", help="Session name")
):
    """Run an existing Soir session."""
    session_app = SessionApp(name)
    session_app.run()
