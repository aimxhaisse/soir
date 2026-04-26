"""Tests for VST plugin runtime helpers."""

import unittest
from unittest.mock import patch

from soir.rt import vst
from soir.rt.errors import VSTNotFoundException


class TestRequire(unittest.TestCase):
    """Test cases for vst.require()."""

    @patch("soir.rt.vst.plugins")
    def test_require_existing_plugin(self, mock_plugins: object) -> None:
        """require() returns silently when the plugin is present."""
        mock_plugins.return_value = [  # type: ignore[attr-defined]
            {"name": "Reverb", "type": "effect"},
            {"name": "Synth", "type": "instrument"},
        ]

        vst.require("Reverb")
        vst.require("Synth")

    @patch("soir.rt.vst.plugins")
    def test_require_missing_plugin_raises(self, mock_plugins: object) -> None:
        """require() raises VSTNotFoundException for unknown plugins."""
        mock_plugins.return_value = [  # type: ignore[attr-defined]
            {"name": "Reverb", "type": "effect"},
        ]

        with self.assertRaises(VSTNotFoundException):
            vst.require("DoesNotExist")

    @patch("soir.rt.vst.plugins")
    def test_require_with_no_plugins(self, mock_plugins: object) -> None:
        """require() raises when the plugin list is empty."""
        mock_plugins.return_value = []  # type: ignore[attr-defined]

        with self.assertRaises(VSTNotFoundException):
            vst.require("Anything")

    @patch("soir.rt.vst.plugins")
    def test_require_is_case_sensitive(self, mock_plugins: object) -> None:
        """require() matches plugin names exactly (case sensitive)."""
        mock_plugins.return_value = [  # type: ignore[attr-defined]
            {"name": "Reverb", "type": "effect"},
        ]

        with self.assertRaises(VSTNotFoundException):
            vst.require("reverb")


if __name__ == "__main__":
    unittest.main()
