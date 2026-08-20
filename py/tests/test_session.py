"""Tests for session management CLI."""

import json
import tempfile
import unittest
from pathlib import Path

from typer.testing import CliRunner

from soir import _resources
from soir.cli.session import session_app


class TestSessionMk(unittest.TestCase):
    """Test cases for `soir session mk`."""

    def test_mk_lists_std_packs_in_config(self) -> None:
        """New sessions list shipped std-* packs in dsp.sample_packs."""
        runner = CliRunner()
        with tempfile.TemporaryDirectory() as tmp:
            result = runner.invoke(session_app, ["mk", str(Path(tmp) / "mysession")])
            self.assertEqual(result.exit_code, 0, result.output)

            config_path = Path(tmp) / "mysession" / "etc" / "config.json"
            self.assertTrue(config_path.is_file())

            config = json.loads(config_path.read_text())
            self.assertEqual(
                config["dsp"]["sample_packs"],
                _resources.resources.std_pack_names(),
            )
            self.assertIn("std-drums", config["dsp"]["sample_packs"])

    def test_mk_creates_layout(self) -> None:
        """Session directory structure and live.py template are created."""
        runner = CliRunner()
        with tempfile.TemporaryDirectory() as tmp:
            session_path = Path(tmp) / "mysession"
            result = runner.invoke(session_app, ["mk", str(session_path)])
            self.assertEqual(result.exit_code, 0, result.output)

            self.assertTrue((session_path / "etc" / "config.json").is_file())
            self.assertTrue((session_path / "lib" / "samples").is_dir())
            self.assertTrue((session_path / "var" / "log").is_dir())
            self.assertTrue((session_path / "live.py").is_file())


if __name__ == "__main__":
    unittest.main()
