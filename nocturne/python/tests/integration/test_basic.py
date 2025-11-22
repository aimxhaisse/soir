"""Basic integration tests for log() function and notifications."""

import unittest
from pathlib import Path
import tempfile
import shutil

from .base import SoirIntegrationTestCase


class TestBasicLog(SoirIntegrationTestCase):
    """Test basic log() functionality."""

    def test_basic_log_ok(self) -> None:
        """Test that log() messages are captured correctly."""
        self.engine.push_code("log('hello, world')")
        self.assertTrue(self.engine.wait_for_notification("hello, world"))

    def test_basic_log_ko(self) -> None:
        """Test that waiting for non-existent messages returns False."""
        self.engine.push_code("log('hello, world')")
        self.assertFalse(self.engine.wait_for_notification("woot"))
