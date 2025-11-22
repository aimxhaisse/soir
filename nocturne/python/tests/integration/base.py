"""Base test class for Soir integration tests."""

import shutil
import tempfile
import unittest
from pathlib import Path

from .soir_test_base import SoirTestEngine
from soir._core import logging


class SoirIntegrationTestCase(unittest.TestCase):
    """
    Base class for all Soir integration tests.

    Provides common setUp and tearDown functionality including:
    - Temporary directory creation and cleanup
    - Soir engine initialization and shutdown
    - Optional debug notification output
    - Configurable engine settings via config_overrides

    Attributes:
        config_overrides: Optional dict to override engine config
                         (e.g., {"initial_bpm": 600, "block_size": 2048})
        debug_notifications: If True, prints all notifications in tearDown
        engine: SoirTestEngine instance available to all test methods
        temp_dir: Path to temporary directory (cleaned up in tearDown)
        log_dir: Path to log directory within temp_dir

    Example:
        class TestMyFeature(SoirIntegrationTestCase):
            config_overrides = {"initial_bpm": 120}
            debug_notifications = True

            def test_something(self):
                self.engine.push_code("log('hello')")
                self.assertTrue(self.engine.wait_for_notification("hello"))
    """

    # Class attributes that can be overridden by subclasses
    config_overrides: dict | None = None
    debug_notifications: bool = False

    def setUp(self):
        """Set up test engine before each test."""
        self.temp_dir = tempfile.mkdtemp()
        self.log_dir = Path(self.temp_dir) / "logs"
        self.engine = SoirTestEngine(
            str(self.log_dir),
            config_overrides=self.config_overrides
        )

    def tearDown(self):
        """Clean up after each test."""
        if self.debug_notifications:
            print('*' * 80)
            for notification in self.engine._notifications:
                print(f"> [{notification}]")
            print('*' * 80)

        self.engine.stop()
        logging.shutdown()
        shutil.rmtree(self.temp_dir, ignore_errors=True)
