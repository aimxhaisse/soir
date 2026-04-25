"""Access to read-only resources shipped inside the Soir wheel.

Production code uses the module-level ``resources`` instance. Tests
substitute a different root by reassigning the attribute (e.g. via
``unittest.mock.patch.object``) with a fresh ``Resources(root=...)``.
"""

from importlib.resources import files
from pathlib import Path


class Resources:
    """Read-only resources shipped with the Soir wheel."""

    def __init__(self, root: Path):
        self.root = root

    @property
    def config_path(self) -> Path:
        return self.root / "etc" / "config.json"

    @property
    def live_template_path(self) -> Path:
        return self.root / "etc" / "live.default.py"

    @property
    def samples_registry_path(self) -> Path:
        return self.root / "lib" / "samples" / "registry.json"

    @property
    def default_pack_dir(self) -> Path:
        return self.root / "lib" / "samples" / "default"


def _wheel_root() -> Path:
    return Path(str(files("soir") / "_data"))


resources = Resources(_wheel_root())
