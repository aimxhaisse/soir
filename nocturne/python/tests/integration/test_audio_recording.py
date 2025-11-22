"""Integration tests for audio recording functionality (sys.record())."""

import time
from pathlib import Path

import soundfile as sf

from .base import SoirIntegrationTestCase


class TestAudioRecording(SoirIntegrationTestCase):
    """Test sys.record() audio recording functionality."""

    config_overrides = {"initial_bpm": 600}

    def test_basic_recording_file_creation(self):
        """Test that sys.record() creates a WAV file."""
        test_file = Path(self.temp_dir) / "test_recording_basic.wav"

        test_file.unlink(missing_ok=True)
        self.assertFalse(test_file.exists())

        self.engine.push_code(f"""
sys.record("{test_file}")
log("recording_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_started"))
        time.sleep(0.5)

        self.engine.push_code("""
log("recording_stopped")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_stopped"))
        time.sleep(0.5)

        self.assertTrue(test_file.exists())
        test_file.unlink(missing_ok=True)

    def test_valid_wav_file_properties(self):
        """Test that recorded WAV file has correct properties."""
        test_file = Path(self.temp_dir) / "test_recording_wav.wav"

        test_file.unlink(missing_ok=True)

        self.engine.push_code(f"""
sys.record("{test_file}")
log("recording_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_started"))
        time.sleep(0.5)

        self.engine.push_code("""
log("recording_stopped")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_stopped"))
        time.sleep(0.5)

        self.assertTrue(test_file.exists())

        info = sf.info(test_file)
        self.assertEqual(info.samplerate, 48000)
        self.assertEqual(info.channels, 2)
        self.assertEqual(info.subtype, "FLOAT")
        self.assertGreater(info.frames, 0)

        test_file.unlink(missing_ok=True)

    def test_recording_stops_after_code_update(self):
        """Test that recording stops when code is updated without sys.record()."""
        test_file = Path(self.temp_dir) / "test_recording_stop.wav"

        test_file.unlink(missing_ok=True)

        self.engine.push_code(f"""
sys.record("{test_file}")
log("recording_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_started"))
        time.sleep(0.5)

        self.engine.push_code("""
log("code_updated_without_record")
""")

        self.assertTrue(
            self.engine.wait_for_notification("code_updated_without_record")
        )
        time.sleep(0.5)

        self.assertTrue(test_file.exists())

        info = sf.info(test_file)
        self.assertGreater(info.frames, 0)

        original_size = test_file.stat().st_size
        time.sleep(0.5)

        current_size = test_file.stat().st_size
        self.assertEqual(original_size, current_size)

        test_file.unlink(missing_ok=True)

    def test_recording_continues_with_same_file(self):
        """Test that recording continues when same file path is used."""
        test_file = Path(self.temp_dir) / "test_recording_continue.wav"

        test_file.unlink(missing_ok=True)

        self.engine.push_code(f"""
sys.record("{test_file}")
log("recording_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_started"))
        time.sleep(0.3)

        self.engine.push_code(f"""
sys.record("{test_file}")
log("recording_continued")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_continued"))
        time.sleep(0.3)

        self.engine.push_code("""
log("recording_stopped")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_stopped"))
        time.sleep(0.3)

        self.assertTrue(test_file.exists())

        info = sf.info(test_file)
        self.assertGreater(info.frames, 1000)

        test_file.unlink(missing_ok=True)

    def test_recording_with_different_files(self):
        """Test that switching to a different file stops first and starts second."""
        test_file1 = Path(self.temp_dir) / "test_recording_file1.wav"
        test_file2 = Path(self.temp_dir) / "test_recording_file2.wav"

        test_file1.unlink(missing_ok=True)
        test_file2.unlink(missing_ok=True)

        self.engine.push_code(f"""
sys.record("{test_file1}")
log("recording_file1_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_file1_started"))
        time.sleep(0.3)

        self.engine.push_code(f"""
sys.record("{test_file2}")
log("recording_file2_started")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_file2_started"))
        time.sleep(0.3)

        self.engine.push_code("""
log("recording_stopped")
""")

        self.assertTrue(self.engine.wait_for_notification("recording_stopped"))
        time.sleep(0.3)

        self.assertTrue(test_file1.exists())
        self.assertTrue(test_file2.exists())

        info1 = sf.info(test_file1)
        info2 = sf.info(test_file2)

        self.assertGreater(info1.frames, 0)
        self.assertGreater(info2.frames, 0)

        test_file1.unlink(missing_ok=True)
        test_file2.unlink(missing_ok=True)
