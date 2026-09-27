"""Unit tests for the tracks module."""

import json
import unittest
from typing import cast

from soir.rt import _dexed
from soir.rt import tracks
from soir.rt.errors import PresetNotFoundException


class TestMkDexed(unittest.TestCase):
    """Test cases for tracks.mk_dexed()."""

    def _extra(self, t: tracks.Track) -> dict[str, object]:
        return cast("dict[str, object]", json.loads(t.extra or "{}"))

    def test_default_patch(self) -> None:
        t = tracks.mk_dexed()
        self.assertEqual(t.instrument, "vst")
        extra = self._extra(t)
        self.assertEqual(extra["plugin"], "Dexed")
        self.assertEqual(
            extra["params"],
            _dexed.params(_dexed.PRESETS[_dexed.DEFAULT_PATCH]),
        )

    def test_named_patch(self) -> None:
        t = tracks.mk_dexed("dark_bass")
        extra = self._extra(t)
        self.assertEqual(extra["plugin"], "Dexed")
        self.assertEqual(extra["params"], _dexed.params(_dexed.PRESETS["dark_bass"]))

    def test_custom_spec(self) -> None:
        spec = {"algo": 1, "ops": [_dexed.op(220.0, level=0.9)]}
        t = tracks.mk_dexed(spec)
        extra = self._extra(t)
        self.assertEqual(extra["params"], _dexed.params(spec))

    def test_unknown_preset_raises(self) -> None:
        with self.assertRaises(PresetNotFoundException):
            tracks.mk_dexed("does_not_exist")

    def test_track_fields(self) -> None:
        t = tracks.mk_dexed("bell", muted=True, volume=0.5, pan=-0.25)
        self.assertTrue(t.muted)
        self.assertEqual(t.volume, 0.5)
        self.assertEqual(t.pan, -0.25)


if __name__ == "__main__":
    unittest.main()
