"""Integration tests for levels API."""

from .base import SoirIntegrationTestCase


class TestLevels(SoirIntegrationTestCase):
    """Test level metering functionality."""

    def test_get_master_levels(self) -> None:
        """Test getting master output levels."""
        code = """
m = levels.get_master_levels()
log(f"master:{m.peak_left:.3f},{m.peak_right:.3f},{m.rms_left:.3f},{m.rms_right:.3f}")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("master:"))

    def test_get_track_levels_empty(self) -> None:
        """Test getting track levels when no tracks exist."""
        code = """
t = levels.get_track_levels()
log(f"count:{len(t)}")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("count:0"))

    def test_get_track_levels_with_track(self) -> None:
        """Test getting track levels after creating a track."""
        code = """
tracks.setup({
    'drums': tracks.mk('sampler'),
})

t = levels.get_track_levels()
log(f"count:{len(t)}")
if 'drums' in t:
    d = t['drums']
    log(f"drums:{d.peak_left:.3f},{d.peak_right:.3f}")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("count:1"))
        self.assertTrue(self.engine.wait_for_notification("drums:"))

    def test_get_track_level_specific(self) -> None:
        """Test getting levels for a specific track."""
        code = """
tracks.setup({
    'bass': tracks.mk('sampler'),
})

bass = levels.get_track_level('bass')
log(f"bass:{bass.peak_left:.3f},{bass.peak_right:.3f}")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("bass:"))

    def test_get_track_level_nonexistent(self) -> None:
        """Test getting levels for a track that doesn't exist."""
        code = """
try:
    levels.get_track_level('nonexistent')
    log("result:no_exception")
except errors.TrackNotFoundException:
    log("result:exception_raised")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("result:exception_raised"))

    def test_levels_are_floats(self) -> None:
        """Test that level values are proper floats."""
        code = """
m = levels.get_master_levels()
log(f"types:{type(m.peak_left).__name__},{type(m.rms_left).__name__}")
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("types:float,float"))
