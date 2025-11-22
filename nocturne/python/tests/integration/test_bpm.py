"""Integration tests for BPM API."""

from .base import SoirIntegrationTestCase


class TestBPM(SoirIntegrationTestCase):
    """Test BPM get and set functionality."""

    def test_get_bpm_ok(self):
        """Test getting and setting BPM values."""
        code = """
bpm.set(120)
log(str(bpm.get()))
bpm.set(60)
log(str(bpm.get()))
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("120.0"))
        self.assertTrue(self.engine.wait_for_notification("60.0"))
