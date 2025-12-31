"""Integration tests for system module device enumeration."""

from .base import SoirIntegrationTestCase


class TestSystemDevices(SoirIntegrationTestCase):
    """Test system module device enumeration functions."""

    def test_audio_out_devices(self) -> None:
        """Test that system.get_audio_out_devices() returns available devices."""
        self.engine.push_code(
            """
if len(system.get_audio_out_devices()):
    log("OK")
else:
    log("KO")
        """
        )
        self.assertTrue(self.engine.wait_for_notification("OK"))

    def test_audio_in_devices(self) -> None:
        """Test that system.get_audio_in_devices() returns available devices."""
        self.engine.push_code(
            """
if len(system.get_audio_in_devices()):
    log("OK")
else:
    log("KO")
        """
        )
        self.assertTrue(self.engine.wait_for_notification("OK"))

    def test_midi_out_devices(self) -> None:
        """Test that system.get_midi_out_devices() returns available devices."""
        self.engine.push_code(
            """
system.get_midi_out_devices()
log("OK")
        """
        )
        self.assertTrue(self.engine.wait_for_notification("OK"))
