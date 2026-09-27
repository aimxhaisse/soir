"""Integration tests for Dexed preset tracks (synth module).

These tests require the Dexed VST3 plug-in to be installed on the
system (see _TEST_PLUGIN_LOCATIONS in base.py). They skip themselves
when the plug-in is not available to the engine.
"""

import time
from pathlib import Path
from typing import ClassVar

from .base import SoirSessionTestCase
from .dsp import read_wav, rms


class TestDexedPresets(SoirSessionTestCase):
    """Dexed preset tracks must produce audio from MIDI notes."""

    config_overrides: ClassVar[dict[str, bool]] = {"vst_scan_at_startup": True}

    def test_preset_track_produces_audio(self) -> None:
        """Notes played on a mk_dexed track must reach the plug-in.

        Uses a sustained, fast-attack preset so the recorded level is
        stable regardless of the audio block size.
        """
        wav_path = Path(self.temp_dir) / "dexed.wav"
        wav_path.unlink(missing_ok=True)

        self.engine.push_code(
            f"""
try:
    if not any(p['name'] == 'Dexed' for p in vst.instruments()):
        log("dexed-audio=skipped")
    else:
        tracks.setup({{'pad': tracks.mk_dexed('brass_stab', volume=1.0)}})

        @loop('pad', beats=8)
        def notes():
            with midi.use_chan(1):
                for n in [45, 48, 52, 55, 60, 64, 67, 72]:
                    midi.note(n, 1.8, 110)
                    sleep(1.0)

        sys.record("{wav_path}")
        log("dexed-audio=record-start")
except Exception as e:
    log(f"dexed-audio=error:{{e}}")
"""
        )
        self.assertTrue(self.engine.wait_for_notification("dexed-audio="))
        for n in self.engine.get_notifications():
            if "dexed-audio=skipped" in n:
                self.skipTest("Dexed plug-in not available to the engine")
            if "dexed-audio=error" in n:
                self.fail(f"Test error: {n}")

        # 8 beats at 120 bpm = 4 s per loop; record two full loops.
        time.sleep(8.0)
        self.engine.push_code('log("dexed-audio=record-stop")')
        self.assertTrue(self.engine.wait_for_notification("dexed-audio=record-stop"))
        time.sleep(0.5)

        self.assertTrue(wav_path.exists())
        data, sr = read_wav(wav_path)
        self.assertGreater(len(data), 0)
        level = rms(data[:, 0])
        self.assertGreater(level, 0.01, f"Dexed preset produced silence (rms={level})")
