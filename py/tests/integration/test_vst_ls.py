"""Integration tests for the 'soir vst ls' CLI command."""

import unittest

from soir.app import app
from typer.testing import CliRunner

from .base import SoirStandaloneTestCase

runner = CliRunner()


class TestVstLsCommand(SoirStandaloneTestCase):
    """Test 'soir vst ls' CLI surface via CliRunner."""

    def test_ls_exits_cleanly(self) -> None:
        """'vst ls' exits with code 0."""
        result = runner.invoke(app, ["vst", "ls"])
        self.assertEqual(result.exit_code, 0, result.output)

    def test_ls_output_contains_table_title(self) -> None:
        """'vst ls' output contains the VST3 Plugins table title."""
        result = runner.invoke(app, ["vst", "ls"])
        self.assertEqual(result.exit_code, 0, result.output)
        self.assertIn("VST3 Plugins", result.output)

    def test_ls_type_filter_effect(self) -> None:
        """'vst ls --type effect' exits cleanly."""
        result = runner.invoke(app, ["vst", "ls", "--type", "effect"])
        self.assertEqual(result.exit_code, 0, result.output)

    def test_ls_type_filter_instrument(self) -> None:
        """'vst ls --type instrument' exits cleanly."""
        result = runner.invoke(app, ["vst", "ls", "--type", "instrument"])
        self.assertEqual(result.exit_code, 0, result.output)

    def test_ls_invalid_type_filter(self) -> None:
        """'vst ls --type foobar' exits with a non-zero code and error message."""
        result = runner.invoke(app, ["vst", "ls", "--type", "foobar"])
        self.assertNotEqual(result.exit_code, 0)
        self.assertIn("foobar", result.output)


if __name__ == "__main__":
    unittest.main()
