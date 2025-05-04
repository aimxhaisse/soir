"""
Session management module for Soir.

This module provides utilities for creating and managing Soir sessions.
"""

import os
import shutil
import textwrap
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
    RadioSet,
    RadioButton,
    Rule,
    Select,
    Static,
    TabbedContent,
    TabPane,
)
from textual.widget import Widget

from soir.system import (
    exec_session,
    get_audio_devices,
)
      


SOIR_DIR = os.getenv("SOIR_DIR")


class SessionApp(App):
    """Control Center for Soir."""

    CSS = """
    Screen {
        padding: 2;
    }

    #overview-content {
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

        self.current_device_id = self.config['soir']['dsp']['output']['audio']['device_id']

    def save(self):
        """Saves the config."""
        self.config['soir']['dsp']['output']['audio']['device_id'] = self.current_device_id
        with open(self.config_path, "w") as f:
            yaml.safe_dump(self.config, f)

    def compose(self) -> ComposeResult:
        """Create child widgets for the app."""

        yield Header(show_clock=True)

        with TabbedContent(initial="overview"):

            with TabPane("Overview", id="overview"):
                overview = textwrap.dedent(
                    f"""
                    # Session **{self.session_name.upper()}**

                    ```
                    Code flows like music
                    Algorithms dance through sound
                    Python sings alive
                    ```
                    """
                )
                yield Markdown(overview, id="overview-content")
                with Container(id="run-button-container"):
                    yield Button("Run", variant="primary", id="run-button")

            with TabPane("Audio", id="audio"):
                audio_devices = get_audio_devices()
                yield Markdown("# Audio Output", id="audio-header")
                with RadioSet(id="audio_devices"):
                    for device_id, name in audio_devices:
                        yield RadioButton(name, value=(self.current_device_id == device_id))

    def on_radio_set_changed(self, event: RadioSet.Changed) -> None:
        """Handle radio button changes."""
        if event.radio_set.id == "audio_devices":
            self.current_device_id = event.index

    def on_button_pressed(self, event: Button.Pressed) -> None:
        """Handle button presses."""
        if event.button.id == "run-button":
            self.save()
            self.exit()


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
    exec_session(name)


@app.command("run")
def run_session(
        name: str = typer.Argument(help="Session name"),
):
    """Run an existing Soir session."""
    session_app = SessionApp(name)
    session_app.load()
    session_app.run()
    exec_session(name)


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
