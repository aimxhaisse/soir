"""Configuration management for Soir.

This module defines Pydantic models for configuration validation and
loading.  Configurations are validated on the Python side using
Pydantic, then serialized to JSON and passed to the C++ engine.
"""

import os
from pathlib import Path

from pydantic import BaseModel, Field

from soir.rt.errors import ConfigurationError


def is_session(path: Path) -> bool:
    """Determine whether a given path is a Soir session directory.

    A session directory is any directory that is NOT the global SOIR_DIR.
    Both a session and SOIR_DIR contain an 'etc/config.json', so we cannot
    rely on file structure alone to tell them apart. Instead, we use the
    invariant that 'session mk' always creates sessions outside of SOIR_DIR,
    making the two mutually exclusive by construction.

    This means: if the path resolves to SOIR_DIR, we are running from the
    global configuration (no session). Any other path is treated as a session.

    Args:
        path: The directory path to test.

    Returns:
        True if path is a session directory, False if it is SOIR_DIR.

    Raises:
        ConfigurationError: If SOIR_DIR is not set.
    """
    return path.resolve() != Path(get_soir_dir()).resolve()


def get_soir_dir() -> str:
    """Get the SOIR_DIR environment variable.

    Returns:
        The SOIR_DIR path

    Raises:
        ConfigurationError: If SOIR_DIR is not set
    """
    soir_dir = os.getenv("SOIR_DIR")
    if not soir_dir:
        raise ConfigurationError("SOIR_DIR environment variable not set")
    return soir_dir


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
    def load_global(cls) -> Config:
        """Load the global configuration from SOIR_DIR.

        Returns:
            Validated Config instance from $SOIR_DIR/etc/config.json

        Raises:
            ConfigurationError: If SOIR_DIR is not set or config file not found
        """
        soir_dir = get_soir_dir()
        return cls.load_from_path(Path(soir_dir) / "etc" / "config.json")

    @classmethod
    def load_from_path(cls, path: str | Path) -> Config:
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
