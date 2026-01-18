"""Configuration management for Soir.

This module defines Pydantic models for configuration validation and
loading.  Configurations are validated on the Python side using
Pydantic, then serialized to JSON and passed to the C++ engine.
"""

import os
from pathlib import Path

from pydantic import BaseModel, Field

from soir.rt.errors import ConfigurationError


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

    class DspConfig(BaseModel):
        """DSP configuration."""

        enable_output: bool = Field(default=True)
        block_size: int = Field(default=4096)

    dsp: DspConfig = Field()
    live: LiveConfig = Field()

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
