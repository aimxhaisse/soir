"""Configuration management for Soir.

This module defines Pydantic models for configuration validation and
loading.  Configurations are validated on the Python side using
Pydantic, then serialized to JSON and passed to the C++ engine.
"""

import os
import shutil
from pathlib import Path

from platformdirs import user_data_dir
from pydantic import BaseModel, Field

from soir import _resources


def is_session(path: Path) -> bool:
    """Determine whether a given path is a Soir session directory.

    A session directory is any directory that is NOT the global SOIR_HOME.
    Both a session and SOIR_HOME contain an 'etc/config.json', so we cannot
    rely on file structure alone to tell them apart. Instead, we use the
    invariant that 'session mk' always creates sessions outside of SOIR_HOME,
    making the two mutually exclusive by construction.

    This means: if the path resolves to SOIR_HOME, we are running from the
    global configuration (no session). Any other path is treated as a session.

    Args:
        path: The directory path to test.

    Returns:
        True if path is a session directory, False if it is SOIR_HOME.
    """
    return path.resolve() != get_soir_home().resolve()


def get_soir_home() -> Path:
    """Resolve the user-data directory.

    Honors the `SOIR_HOME` environment variable if set; otherwise falls
    back to the platform's user data directory.

    Returns:
        Path to the Soir home directory.
    """
    env = os.getenv("SOIR_HOME")
    if env:
        return Path(env)
    return Path(user_data_dir("soir"))


def ensure_soir_home() -> Path:
    """First-run bootstrap of $SOIR_HOME.

    Creates the directory layout, seeds the default config from the
    package-shipped resources, and exports SOIR_HOME into the process
    environment so the C++ extension's getenv() sees it.

    Returns:
        Path to the resolved Soir home directory.
    """
    home = get_soir_home()
    home.mkdir(parents=True, exist_ok=True)
    os.environ["SOIR_HOME"] = str(home)

    etc = home / "etc"
    etc.mkdir(exist_ok=True)
    config_path = etc / "config.json"
    if not config_path.exists():
        shutil.copy(_resources.resources.config_path, config_path)

    samples = home / "lib" / "samples"
    samples.mkdir(parents=True, exist_ok=True)

    (home / "var" / "log").mkdir(parents=True, exist_ok=True)

    default_pack_json = samples / "default.pack.json"
    default_pack_src = _resources.resources.default_pack_dir
    if not default_pack_json.exists() and default_pack_src.exists():
        shutil.copytree(default_pack_src, samples / "default")
        shutil.copy(default_pack_src / "default.pack.json", default_pack_json)

    return home


class Config(BaseModel):
    """Soir configuration."""

    class LiveConfig(BaseModel):
        """Live configuration."""

        directory: str = Field(default=".")
        initial_bpm: int = Field(default=120)

    class DspConfig(BaseModel):
        """DSP configuration."""

        enable_output: bool = Field(default=True)
        enable_streaming: bool = Field(default=False)
        streaming_bitrate: int = Field(default=128000)
        streaming_port: int = Field(default=5001)
        block_size: int = Field(default=4096)
        audio_output_device: str = Field(default="")
        sample_directory: str = Field(default="")
        sample_packs: list[str] = Field(default_factory=list)

    class CastConfig(BaseModel):
        """Cast configuration."""

        enabled: bool = Field(default=False)
        port: int = Field(default=5002)

    dsp: DspConfig = Field()
    live: LiveConfig = Field()
    cast: CastConfig = Field(default_factory=CastConfig)

    @classmethod
    def load_global(cls) -> "Config":
        """Load the global configuration from $SOIR_HOME.

        Returns:
            Validated Config instance from $SOIR_HOME/etc/config.json
        """
        return cls.load_from_path(get_soir_home() / "etc" / "config.json")

    @classmethod
    def load_from_path(cls, path: str | Path) -> "Config":
        """Load configuration from a JSON file.

        Args:
            path: Path to the JSON configuration file

        Returns:
            Validated Config instance
        """
        with open(path) as f:
            return cls.model_validate_json(f.read())

    def save_to_path(self, path: str | Path) -> None:
        """Save configuration to a JSON file.

        Args:
            path: Path to write the JSON configuration file
        """
        with open(path, "w") as f:
            f.write(self.model_dump_json(indent=4))
