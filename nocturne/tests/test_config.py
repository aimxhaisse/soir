"""Tests for configuration management."""

import json
import tempfile
import unittest
from pathlib import Path

from soir.config import Config


class TestConfig(unittest.TestCase):
    """Test cases for configuration management."""

    def test_audio_config_defaults(self):
        """Test AudioConfig with default values."""
        config = Config(dsp={}, live={})
        self.assertEqual(config.dsp.block_size, 4096)
        self.assertEqual(config.live.directory, ".")

    def test_app_config_from_json(self):
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

    def test_load_from_path(self):
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


if __name__ == "__main__":
    unittest.main()
