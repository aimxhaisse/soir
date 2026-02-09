"""Base test class for Soir integration tests."""

import shutil
import tempfile
import unittest
from pathlib import Path
from typing import Any

from .soir_test_base import SoirTestEngine
from soir._bindings import logging


class SoirIntegrationTestCase(unittest.TestCase):
    """Base class for all Soir integration tests."""

    config_overrides: dict[str, Any] | None = None
    debug_notifications: bool = False
    debug_logging: bool = False

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
        self.engine.push_code("""\
reset()
log("reset done")
""")
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

        logging.shutdown()

        shutil.rmtree(self.temp_dir, ignore_errors=True)
