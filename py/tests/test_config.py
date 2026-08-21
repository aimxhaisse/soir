"""Tests for configuration management."""

import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from soir import _resources
from soir.config import Config, ensure_soir_home


class TestConfig(unittest.TestCase):
    """Test cases for configuration management."""

    def test_audio_config_defaults(self) -> None:
        """Test AudioConfig with default values."""
        config = Config(dsp=Config.DspConfig(), live=Config.LiveConfig())
        self.assertEqual(config.dsp.block_size, 4096)
        self.assertEqual(config.live.directory, ".")

    def test_app_config_from_json(self) -> None:
        """Test deserialization from JSON."""
        json_str = """
        {
            "dsp": {
                "block_size": 8192
            },
            "live": {
                 "directory": "/live"
            }
        }
        """
        config = Config.model_validate_json(json_str)

        self.assertEqual(config.dsp.block_size, 8192)
        self.assertEqual(config.live.directory, "/live")

    def test_load_from_path(self) -> None:
        """Test loading configuration from file."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            config_data = {
                "dsp": {"enable_output": False, "block_size": 2048},
                "live": {"directory": "/music"},
            }

            config_file = Path(tmp_dir) / "config.json"
            config_file.write_text(json.dumps(config_data))

            config = Config.load_from_path(config_file)

            self.assertFalse(config.dsp.enable_output)
            self.assertEqual(config.dsp.block_size, 2048)


class TestEnsureSoirHome(unittest.TestCase):
    """Test first-run bootstrap of $SOIR_HOME."""

    def test_seeds_shipped_std_packs(self) -> None:
        """Shipped std-* packs are copied to $SOIR_HOME/lib/samples."""
        with tempfile.TemporaryDirectory() as tmp:
            home = Path(tmp)
            with mock.patch.dict(os.environ, {"SOIR_HOME": str(home)}):
                result = ensure_soir_home()

            self.assertEqual(result, home)
            samples = home / "lib" / "samples"
            self.assertTrue((samples / "std-drums" / "kick-808.wav").is_file())
            self.assertTrue((samples / "std-drums.pack.json").is_file())

            # A second call must be a no-op for already-seeded packs.
            with mock.patch.dict(os.environ, {"SOIR_HOME": str(home)}):
                ensure_soir_home()
            self.assertTrue((samples / "std-drums.pack.json").is_file())

    def test_std_pack_names(self) -> None:
        """The wheel ships at least the std-drums pack."""
        self.assertIn("std-drums", _resources.resources.std_pack_names())


if __name__ == "__main__":
    unittest.main()
