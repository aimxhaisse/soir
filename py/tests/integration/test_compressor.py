"""Integration tests for the compressor FX and sidechain (fx.mk_compressor)."""

import shutil
import tempfile
import time
from pathlib import Path
from typing import Any, ClassVar

import numpy as np
import soundfile as sf  # type: ignore[import-untyped]

from .base import SoirSessionTestCase

_PROJECT_ROOT = Path(__file__).parent.parent.parent.parent

# The sample packs shipped with the repository, installed once in a
# temporary directory with the installed layout (pack JSON at the top
# level of the sample directory). Read-only: shared by all audible
# compressor tests, never cleaned up.
_TEST_PACKS_DIR: Path | None = None


def _test_packs_dir() -> Path:
    """Directory of the test sample packs in the installed layout."""
    global _TEST_PACKS_DIR

    if _TEST_PACKS_DIR is None:
        dest = Path(tempfile.mkdtemp())
        src = _PROJECT_ROOT / "lib" / "samples"
        for pack in ("std-drums", "std-pads"):
            shutil.copytree(src / pack, dest / pack)
            pack_json = src / pack / f"{pack}.pack.json"
            (dest / f"{pack}.pack.json").write_text(pack_json.read_text())
        _TEST_PACKS_DIR = dest

    return _TEST_PACKS_DIR


def rms_of_segment(
    data: np.ndarray, start: float, end: float, sample_rate: int
) -> float:
    """RMS of a [start, end] time window of a mono signal."""
    segment = data[int(start * sample_rate) : int(end * sample_rate)]
    return float(np.sqrt(np.mean(np.square(segment))))


# Kick on the left, pads on the right, for 16 beats (8 s at 120 bpm).
# The kick plays for the first 4 beats, the pads play continuously, so
# the recording has a ducking window (0.3-1.9 s) and a quiet window
# (4.5-7.5 s). The placeholders are replaced by _record().
_KICK_ON_PADS_CODE = """
tracks.setup({
    'kick': tracks.mk_sampler(pan=-1.0__KICK_EXTRA__),
    'pads': tracks.mk_sampler(pan=1.0, fxs=__PADS_FXS__),
})

k = sampler.new('std-drums')
p = sampler.new('std-pads')

@loop(track='kick', beats=16, align=False)
def kick_loop():
    for i in range(4):
        k.play('kick-808', amp=1.0)
        sleep(1)
    sleep(12)

@loop(track='pads', beats=16, align=False)
def pads_loop():
    for i in range(2):
        p.play('pad-sine', amp=0.5)
        sleep(8)

sys.record("WAV_PATH")
log("compressor-record-start")
"""

_DUCK_FX = (
    "{'duck': fx.mk_compressor(source='kick', threshold=0.05, ratio=20.0,"
    " attack=0.001, release=0.35)}"
)


class TestCompressorSetup(SoirSessionTestCase):
    """Test fx.mk_compressor() setup/layout round-trip."""

    def test_compressor_layout_round_trip(self) -> None:
        """A compressor round-trips through setup()/layout() idempotently."""
        self.engine.push_code(
            """
tracks.setup({
    'kick': tracks.mk_sampler(),
    'pads': tracks.mk_sampler(fxs={
        'duck': fx.mk_compressor(source='kick', threshold=0.3, ratio=8.0,
                                 attack=0.001, release=0.3, knee=3.0,
                                 makeup=1.5, wet=0.5),
        'comp': fx.mk_compressor(),
    }),
})

layout = tracks.layout()
log(str(layout['pads']))
log(str(layout['pads'].fxs['duck'].extra))
log(str(layout['pads'].fxs['comp'].extra))

# Idempotent round-trip: the layout feeds back into setup().
setup_ok = tracks.setup(layout)
log('setup-ok ' + str(setup_ok))
layout = tracks.layout()
log(str(layout['pads']))
log(str(layout['pads'].fxs['duck'].extra))
"""
        )

        duck_extra = (
            '{"source": "kick", "threshold": 0.3, "ratio": 8.0, '
            '"attack": 0.001, "release": 0.3, "knee": 3.0, "makeup": 1.5, '
            '"wet": 0.5}'
        )
        default_extra = (
            '{"source": "self", "threshold": 0.25, "ratio": 4.0, '
            '"attack": 0.005, "release": 0.15, "knee": 6.0, "makeup": 1.0, '
            '"wet": 1.0}'
        )
        track_repr = (
            "Track(name=pads, instrument=sampler, muted=False, "
            "volume=1.0, pan=0.0, fxs=['compressor', 'compressor'])"
        )

        self.assertTrue(self.engine.wait_for_notification(track_repr))
        # Source and parameters are preserved in the extra JSON.
        self.assertTrue(self.engine.wait_for_notification(duck_extra))
        # The default source serializes to "self".
        self.assertTrue(self.engine.wait_for_notification(default_extra))
        # The second setup() of the same layout succeeds (idempotent)...
        self.assertTrue(self.engine.wait_for_notification("setup-ok True"))
        # ...and the round-tripped layout is unchanged.
        self.assertTrue(self.engine.wait_for_notification(track_repr))
        self.assertTrue(self.engine.wait_for_notification(duck_extra))

    def test_compressor_control_round_trip(self) -> None:
        """A control bound to a compressor parameter round-trips."""
        self.engine.push_code(
            """
ctrls.mk_val('duck_ratio', 4.0)
tracks.setup({
    'kick': tracks.mk_sampler(),
    'pads': tracks.mk_sampler(fxs={
        'duck': fx.mk_compressor(source='kick', ratio=ctrl('duck_ratio')),
    }),
})

layout = tracks.layout()
log(str(layout['pads'].fxs['duck'].extra))

# Round-trip through setup() with the control name in the extra JSON.
tracks.setup(layout)
log(str(tracks.layout()['pads'].fxs['duck'].extra))
"""
        )

        control_extra = (
            '{"source": "kick", "threshold": 0.25, "ratio": "duck_ratio", '
            '"attack": 0.005, "release": 0.15, "knee": 6.0, "makeup": 1.0, '
            '"wet": 1.0}'
        )
        self.assertTrue(self.engine.wait_for_notification(control_extra))
        self.assertTrue(self.engine.wait_for_notification(control_extra))


class _CompressorAudioTestBase(SoirSessionTestCase):
    """Base for audible compressor tests.

    Points the engine config at the sample packs shipped with the
    repository, installed in a temporary directory with the installed
    layout (pack JSON at the top level of the sample directory).
    """

    config_overrides: ClassVar[dict[str, Any]] = {
        "initial_bpm": 120,
        "sample_directory": str(_test_packs_dir()),
        "sample_packs": ["std-drums", "std-pads"],
    }


class TestCompressorSidechain(_CompressorAudioTestBase):
    """Test audible sidechain behaviour, recorded on the master."""

    def _record(self, kick_extra: str, pads_fxs: str, name: str) -> Path:
        """Push a recording variant of the kick-on-pads code, wait for it
        to play out, stop it with a code update, and return the WAV path."""
        wav_path = Path(self.temp_dir) / name
        wav_path.unlink(missing_ok=True)

        code = (
            _KICK_ON_PADS_CODE.replace("__KICK_EXTRA__", kick_extra)
            .replace("__PADS_FXS__", pads_fxs)
            .replace("WAV_PATH", str(wav_path))
        )
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("compressor-record-start"))
        time.sleep(9.5)

        self.engine.push_code('log("compressor-record-stop")')
        self.assertTrue(self.engine.wait_for_notification("compressor-record-stop"))
        time.sleep(0.5)

        self.assertTrue(wav_path.exists())
        return wav_path

    def test_sidechain_ducks_pads_while_kick_is_playing(self) -> None:
        """With source='kick', the pads are attenuated while the kick
        plays, and unchanged when the kick is silent."""
        ducked = self._record("", _DUCK_FX, "sidechain_ducked.wav")
        reference = self._record("", "{}", "sidechain_reference.wav")

        def analyse(path: Path) -> dict[str, float]:
            data, sr = sf.read(path, always_2d=True)
            left = data[:, 0]
            right = data[:, 1]
            return {
                "kick": rms_of_segment(left, 0.3, 1.9, sr),
                "pads_ducked": rms_of_segment(right, 0.3, 1.9, sr),
                "pads_silent": rms_of_segment(right, 4.5, 7.5, sr),
            }

        ducked_levels = analyse(ducked)
        reference_levels = analyse(reference)

        # Sanity: the kick is audible (left) and the pads are audible
        # (right) in both recordings, in the quiet window.
        for levels in (ducked_levels, reference_levels):
            self.assertGreater(levels["kick"], 0.01)
            self.assertGreater(levels["pads_silent"], 0.01)

        # With the sidechain compressor, the pads are measurably lower
        # while the kick plays...
        self.assertLess(
            ducked_levels["pads_ducked"],
            0.6 * ducked_levels["pads_silent"],
            f"pads not ducked: {ducked_levels}",
        )

        # ...and the same window is untouched without the compressor.
        self.assertGreater(
            reference_levels["pads_ducked"],
            0.6 * reference_levels["pads_silent"],
            f"pads unexpectedly ducked: {reference_levels}",
        )
        self.assertLess(
            reference_levels["pads_ducked"],
            1.7 * reference_levels["pads_silent"],
            f"pads level inconsistent: {reference_levels}",
        )

    def test_ghost_trigger_muted_source_still_ducks(self) -> None:
        """A muted source track keeps driving the sidechain (the tap is
        pre-mute) while the master contains none of its content."""
        ghost = self._record(", muted=True", _DUCK_FX, "sidechain_ghost.wav")

        data, sr = sf.read(ghost, always_2d=True)
        left = data[:, 0]
        right = data[:, 1]

        # The (muted) kick panned hard left is not in the master at all.
        self.assertLess(rms_of_segment(left, 0.3, 1.9, sr), 1e-6)

        # The pads are still ducked by the inaudible trigger.
        pads_ducked = rms_of_segment(right, 0.3, 1.9, sr)
        pads_silent = rms_of_segment(right, 4.5, 7.5, sr)
        self.assertGreater(pads_silent, 0.01)
        self.assertLess(
            pads_ducked,
            0.6 * pads_silent,
            f"muted source did not duck the pads: {pads_ducked} vs " f"{pads_silent}",
        )


class TestCompressorMasterSource(_CompressorAudioTestBase):
    """Test source='master': a track ducked by the whole mix."""

    _MASTER_CODE = """
tracks.setup({
    'bass': tracks.mk_sampler(pan=-1.0),
    'pads': tracks.mk_sampler(pan=1.0, fxs=__PADS_FXS__),
})

b = sampler.new('std-pads')
p = sampler.new('std-pads')

@loop(track='bass', beats=8, align=False)
def bass_loop():
    b.play('pad-sine', amp=0.8)
    sleep(8)

@loop(track='pads', beats=8, align=False)
def pads_loop():
    p.play('pad-sine', amp=0.4)
    sleep(8)

sys.record("WAV_PATH")
log("compressor-record-start")
"""

    def _record(self, pads_fxs: str, name: str) -> Path:
        wav_path = Path(self.temp_dir) / name
        wav_path.unlink(missing_ok=True)

        code = self._MASTER_CODE.replace("__PADS_FXS__", pads_fxs).replace(
            "WAV_PATH", str(wav_path)
        )
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("compressor-record-start"))
        time.sleep(5.0)

        self.engine.push_code('log("compressor-record-stop")')
        self.assertTrue(self.engine.wait_for_notification("compressor-record-stop"))
        time.sleep(0.5)

        self.assertTrue(wav_path.exists())
        return wav_path

    def test_master_source_ducks_track_on_mix(self) -> None:
        """With source='master', the pads track is attenuated by the
        loud mix (the bass track) and not by itself alone."""
        master_duck_fx = (
            "{'duck': fx.mk_compressor(source='master', threshold=0.08,"
            " ratio=20.0, attack=0.001, release=0.5)}"
        )
        ducked = self._record(master_duck_fx, "master_ducked.wav")
        reference = self._record("{}", "master_reference.wav")

        def right_rms(path: Path) -> float:
            data, sr = sf.read(path, always_2d=True)
            return rms_of_segment(data[:, 1], 0.5, 3.5, sr)

        ducked_rms = right_rms(ducked)
        reference_rms = right_rms(reference)

        self.assertGreater(reference_rms, 0.01)
        self.assertLess(
            ducked_rms,
            0.5 * reference_rms,
            f"master-source compression had no effect: {ducked_rms} vs "
            f"{reference_rms}",
        )
