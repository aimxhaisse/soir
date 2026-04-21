"""Base test classes for Soir integration tests."""

import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path
from typing import Any, ClassVar

from .soir_test_base import SoirTestEngine

_PROJECT_ROOT = Path(__file__).parent.parent.parent.parent

_STANDALONE_TEST_CONFIG = {
    "dsp": {
        "enable_output": False,
        "enable_streaming": False,
        "streaming_bitrate": 128000,
        "streaming_port": 5001,
        "block_size": 4096,
        "sample_directory": str(_PROJECT_ROOT / "lib" / "samples"),
        "sample_packs": [],
    },
    "live": {
        "directory": ".",
        "initial_bpm": 120,
    },
}


class SoirSessionTestCase(unittest.TestCase):
    """Base class for engine-level integration tests using push_code."""

    config_overrides: ClassVar[dict[str, Any] | None] = None
    debug_notifications: ClassVar[bool] = False
    debug_logging: ClassVar[bool] = False

    def setUp(self) -> None:
        """Set up test engine before each test."""
        self.temp_dir = tempfile.mkdtemp()
        self.log_dir = Path(self.temp_dir) / "logs"
        self.engine = SoirTestEngine(
            str(self.log_dir), config_overrides=self.config_overrides
        )
        self.resetPythonInternals()

    def resetPythonInternals(self) -> None:
        """Reset internal state of Soir modules."""
        self.engine.push_code(
            """\
reset()
log("reset done")
"""
        )
        self.engine.wait_for_notification("reset done")

    def tearDown(self) -> None:
        """Clean up after each test."""
        self.engine.stop()

        if self.debug_notifications:
            print("*" * 80)
            for notification in self.engine._notifications:
                print(f"> [{notification}]")
            print("*" * 80)

        if self.debug_logging:
            print("=" * 80)
            log_files = sorted(self.log_dir.glob("*.log"))
            for log_file in log_files:
                print(f"-- Log file: {log_file} --")
                with log_file.open("r") as f:
                    print(f.read())
            print("=" * 80)

        shutil.rmtree(self.temp_dir, ignore_errors=True)


class SoirStandaloneTestCase(unittest.TestCase):
    """Base class for CLI tests that use EngineManager.initialize_standalone().

    Sets up a temporary SOIR_DIR skeleton with etc/config.json and var/log/,
    isolated from the real installation. SOIR_DIR is restored after each test.
    """

    def setUp(self) -> None:
        """Set up isolated SOIR_DIR skeleton before each test."""
        self.temp_dir = tempfile.mkdtemp()
        soir_dir = Path(self.temp_dir)
        (soir_dir / "etc").mkdir()
        (soir_dir / "var" / "log").mkdir(parents=True)
        (soir_dir / "etc" / "config.json").write_text(
            json.dumps(_STANDALONE_TEST_CONFIG)
        )
        self._saved_soir_dir: str | None = os.environ.get("SOIR_DIR")
        os.environ["SOIR_DIR"] = str(soir_dir)

    def tearDown(self) -> None:
        """Restore SOIR_DIR and clean up temp directory after each test."""
        if self._saved_soir_dir is not None:
            os.environ["SOIR_DIR"] = self._saved_soir_dir
        else:
            os.environ.pop("SOIR_DIR", None)
        shutil.rmtree(self.temp_dir, ignore_errors=True)
