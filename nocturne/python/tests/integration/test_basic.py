"""Basic integration tests for log() function and notifications."""

import unittest
from pathlib import Path
import tempfile
import shutil

from soir._core import logging
from .soir_test_base import SoirTestEngine


class TestBasicLog(unittest.TestCase):
    """Test basic log() functionality."""

    def setUp(self):
        """Set up test engine before each test."""
        self.temp_dir = tempfile.mkdtemp()
        self.log_dir = Path(self.temp_dir) / "logs"
        self.engine = SoirTestEngine(str(self.log_dir))

    def tearDown(self):
        """Clean up after each test."""
        self.engine.stop()
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_basic_log_ok(self):
        """Test that log() messages are captured correctly."""
        self.engine.push_code("log('hello, world')")
        self.assertTrue(self.engine.wait_for_notification("hello, world"))

    def test_basic_log_ko(self):
        """Test that waiting for non-existent messages returns False."""
        self.engine.push_code("log('hello, world')")
        self.assertFalse(self.engine.wait_for_notification("woot"))
