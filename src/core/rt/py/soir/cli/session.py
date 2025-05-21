"""
Session management module for Soir.

This module provides utilities for creating and managing Soir sessions.
"""

import os
import shutil
import textwrap
import typer
from typing import Optional
import glob
import yaml
import re

from textual import events
from textual.app import (
    App,
    ComposeResult,
    RenderResult,
)
from textual.containers import (
    Container,
    Horizontal,
    ScrollableContainer,
)
from textual.reactive import reactive
from textual.widgets import (
    Button,
    Checkbox,
    Header,
    Label,
    Markdown,
    RadioSet,
    RadioButton,
    Rule,
    Select,
    SelectionList,
    Static,
    TabbedContent,
    TabPane,
)
from textual.widget import Widget

from .utils import (
    expand_env_vars,
)
from soir.system import (
    exec_session,
    get_audio_out_devices,
)


SOIR_DIR = os.getenv("SOIR_DIR")


class SessionApp(App[bool]):
    """Control Center for Soir."""

    CSS = """
    Screen {
        padding: 2;
    }

    Header {
        dock: top;
        height: 3;
        content-align: center middle;
        color: white;
    }
    
    .soir-buttons {
        margin: 1 0;
    }

    RadioSet {
        border: none;
        width: 100%;
    }

    SelectionList {
        border: none;
        width: 100%;
    }
    """

    def __init__(self, name: str):
        super().__init__()

        if not SOIR_DIR:
            raise typer.BadParameter("SOIR_DIR environment variable not set.")

        self.session_name = name
        self.title = "soir"
        self.sub_title = " https://soir.dev"
        self.theme = "tokyo-night"

        self.config_path = os.path.join(name, "etc", "config.yaml")
        self.live_path = os.path.join(name, "live.py")
        self.config = None

        self.available_devices: list[tuple[int, str]] = []
        self.current_device_id = 0

        self.available_sample_packs: dict[str, dict] = {}
        self.current_sample_packs: list[str] = []

    def on_mount(self) -> None:
        """Called when the app is mounted."""
        self.screen.styles.border = ("heavy", "white")

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
            os.path.join(self.session_name, "live.py"),
        )

    def load(self):
        """Loads a session."""
        self.load_config()
        self.load_devices()
        self.load_sample_packs()

    def load_config(self):
        """Loads the config file."""
        if not os.path.exists(self.session_name):
            raise typer.BadParameter(f"Session {self.session_name} does not exist.")
        if not os.path.exists(self.config_path):
            raise typer.BadParameter(
                f"No config file found in session {self.session_name}."
            )
        with open(self.config_path, "r") as f:
            self.config = yaml.safe_load(f)
        if not self.config:
            raise typer.BadParameter(
                f"Invalid config file in session {self.session_name}."
            )

    def load_sample_packs(self):
        """Loads sample packs from SOIR_DIR & config."""
        self.current_sample_packs = self.config["soir"]["dsp"].get("sample_packs", [])
        self.available_sample_packs = {}
        sample_dir = expand_env_vars(self.config["soir"]["dsp"]["sample_directory"])
        pack_files = glob.glob(os.path.join(sample_dir, "*.pack.yaml"))
        for pack_file in pack_files:
            try:
                with open(pack_file, "r") as f:
                    pack = yaml.safe_load(f)
                if pack and "name" in pack:
                    self.available_sample_packs[pack["name"]] = pack
            except Exception:
                pass

    def load_devices(self):
        """Loads audio devices from the system & config."""
        self.current_device_id = self.config["soir"]["dsp"]["output"]["audio"][
            "device_id"
        ]
        self.available_devices = get_audio_out_devices()

    def save(self):
        """Saves the config."""
        self.config["soir"]["dsp"]["output"]["audio"][
            "device_id"
        ] = self.current_device_id

        if "sample_packs" not in self.config["soir"]["dsp"]:
            self.config["soir"]["dsp"]["sample_packs"] = []
        self.config["soir"]["dsp"]["sample_packs"] = self.current_sample_packs

        with open(self.config_path, "w") as f:
            yaml.safe_dump(self.config, f)

    def compose(self) -> ComposeResult:
        """Create child widgets for the app."""

        yield Header(show_clock=True)

        with TabbedContent(initial="overview"):

            with TabPane("Overview", id="overview"):
                overview = textwrap.dedent(
                    f"""
                    [b]Session [$accent]{self.session_name.upper()}[/$accent][/b]

                    > [i]Code flows like music[/i]
                    > [i]Algorithms dance through sound[/i]
                    > [i]Python sings alive[/i]
                    """
                )
                yield Static(overview)
                with Container(classes="soir-buttons"):
                    with Horizontal():
                        yield Button("Exit", id="exit-button")
                        yield Button("Run", variant="primary", id="run-button")

            with TabPane("Audio", id="audio"):
                audio_header = textwrap.dedent(
                    f"""
                [b][u]Select Output Device[/u][/b]
                """
                )
                yield Static(audio_header)

                with RadioSet(id="audio-devices"):
                    for device_id, name in self.available_devices:
                        yield RadioButton(
                            name, value=(self.current_device_id == device_id)
                        )

            with TabPane("Samples", id="samples"):
                sample_header = textwrap.dedent(
                    f"""
                [b][u]Select Sample Packs[/u][/b]
                """
                )
                yield Static(sample_header)

                packs = []
                for pack_name, pack in self.available_sample_packs.items():
                    sample_count = len(pack.get("samples", []))
                    is_selected = pack_name in self.current_sample_packs
                    packs.append(
                        (
                            f"{pack_name} ({sample_count} samples)",
                            pack_name,
                            is_selected,
                        )
                    )

                yield SelectionList[str](*packs, id="samples")

    def on_radio_set_changed(self, event: RadioSet.Changed) -> None:
        """Handle radio button changes."""
        if event.radio_set.id == "audio-devices":
            self.current_device_id = event.index

    def on_selection_list_selected_changed(
        self, event: SelectionList.SelectedChanged
    ) -> None:
        """Handle selection list changes."""
        if event.selection_list.id == "samples":
            self.current_sample_packs = event.selection_list.selected

    def on_button_pressed(self, event: Button.Pressed) -> None:
        """Handle button presses."""
        if event.button.id == "run-button":
            self.save()
            self.exit(True)
        if event.button.id == "exit-button":
            self.exit(False)


app = typer.Typer(help="Session management commands", no_args_is_help=True)


@app.command("mk")
def init_session(
    name: str = typer.Argument(help="Session name"),
):
    """Create a new Soir session."""
    session_app = SessionApp(name)
    session_app.new()
    session_app.load()
    do_run = session_app.run()
    if do_run:
        exec_session(name)


@app.command("run")
def run_session(
    name: str = typer.Argument(help="Session name"),
):
    """Run an existing Soir session."""
    session_app = SessionApp(name)
    session_app.load()
    do_run = session_app.run()
    if do_run:
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
