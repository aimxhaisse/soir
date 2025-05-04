"""
Session management module for Soir.

This module provides utilities for creating and managing Soir sessions.
"""

import os
import shutil
import typer
from typing import Optional
import yaml

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


SOIR_DIR = os.getenv("SOIR_DIR")

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

        if not SOIR_DIR:
            raise typer.BadParameter("SOIR_DIR environment variable not set.")

        self.session_name = name
        self.title = "soir"
        self.sub_title = " https://soir.dev"
        self.theme = "nord"

        self.config_path = os.path.join(name, "etc", "config.yaml")
        self.live_path = os.path.join(name, "live.py")
        self.config = None

    def new(self):
        """Creates a new session."""
        if os.path.exists(self.session_name):
            raise typer.BadParameter(f"Session {self.session_name} already exists.")

        os.makedirs(os.path.join(self.session_name, "etc"))

        shutil.copy(
            os.path.join(SOIR_DIR, "etc", "session.yaml"),
            self.config_path,
        )
        shutil.copy(
            os.path.join(SOIR_DIR, "templates", "default.py"),
            os.path.join(self.session_name, "live.py")
        )

    def load(self):
        """Loads a session."""
        if not os.path.exists(self.session_name):
            raise typer.BadParameter(f"Session {self.session_name} does not exist.")
        if not os.path.exists(self.config_path):
            raise typer.BadParameter(f"No config file found in session {self.session_name}.")
        with open(self.config_path, "r") as f:
            self.config = yaml.safe_load(f)
        if not self.config:
            raise typer.BadParameter(f"Invalid config file in session {self.session_name}.")

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


@app.command("new")
def init_session(
    name: str = typer.Argument(help="Session name"),
):
    """Create a new Soir session."""
    session_app = SessionApp(name)
    session_app.new()
    session_app.load()
    session_app.run()


@app.command("run")
def run_session(
        name: str = typer.Argument(help="Session name"),
):
    """Run an existing Soir session."""
    session_app = SessionApp(name)
    session_app.load()
    session_app.run()


@app.command("rm")
def rm_session(
        name: str = typer.Argument(help="Session name"),
):
    """Remove a Soir session."""
    session_app = SessionApp(name)

    # This is to make sure this is a soir session and not a random
    # directory. This will throw an error if invalid.
    session_app.load()

    shutil.rmtree(name)
