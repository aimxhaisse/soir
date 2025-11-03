"""Tests for code watcher."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from watchdog import events

from soir.config import Config
from soir.watcher import LiveCodeUpdateHandler, Watcher, reload_code


class TestReloadCode(unittest.TestCase):
    """Test cases for reload_code function."""

    def test_reload_code_single_file(self) -> None:
        """Test reloading code from directory with single file."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Create a single Python file
            test_file = Path(tmp_dir) / "test.py"
            test_file.write_text("print('hello')")

            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            self.assertEqual(content, "print('hello')")

    def test_reload_code_multiple_files(self) -> None:
        """Test reloading code from directory with multiple files."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Create multiple Python files
            (Path(tmp_dir) / "a.py").write_text("# file a\n")
            (Path(tmp_dir) / "b.py").write_text("# file b\n")
            (Path(tmp_dir) / "c.py").write_text("# file c\n")

            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            # Files should be sorted alphabetically
            self.assertEqual(content, "# file a\n# file b\n# file c\n")

    def test_reload_code_nested_directories(self) -> None:
        """Test reloading code from nested directory structure."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Create nested directory structure
            sub_dir = Path(tmp_dir) / "sub"
            sub_dir.mkdir()
            (Path(tmp_dir) / "root.py").write_text("# root\n")
            (sub_dir / "nested.py").write_text("# nested\n")

            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            # Should include files from both root and subdirectory
            self.assertIn("# root\n", content)
            self.assertIn("# nested\n", content)

    def test_reload_code_ignores_non_python_files(self) -> None:
        """Test that non-Python files are ignored."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Create Python and non-Python files
            (Path(tmp_dir) / "test.py").write_text("# python\n")
            (Path(tmp_dir) / "test.txt").write_text("# text\n")
            (Path(tmp_dir) / "test.md").write_text("# markdown\n")

            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            # Should only include Python file
            self.assertEqual(content, "# python\n")

    def test_reload_code_empty_directory(self) -> None:
        """Test reloading code from empty directory."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            self.assertEqual(content, "")

    def test_reload_code_preserves_file_order(self) -> None:
        """Test that files are concatenated in sorted order."""
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Create files with names that would have different sort orders
            (Path(tmp_dir) / "z.py").write_text("z")
            (Path(tmp_dir) / "a.py").write_text("a")
            (Path(tmp_dir) / "m.py").write_text("m")

            callback = Mock()
            reload_code(callback, tmp_dir)

            callback.assert_called_once()
            content = callback.call_args[0][0]
            # Should be in alphabetical order by full path
            self.assertEqual(content, "amz")


class TestLiveCodeUpdateHandler(unittest.TestCase):
    """Test cases for LiveCodeUpdateHandler."""

    def setUp(self) -> None:
        """Set up test fixtures."""
        self.directory = "/test/dir"
        self.callback = Mock()
        self.handler = LiveCodeUpdateHandler(self.directory, self.callback)

    @patch("soir.watcher.reload_code")
    def test_on_modified_python_file(self, mock_reload: Mock) -> None:
        """Test that modifying a Python file triggers reload."""
        event = events.FileModifiedEvent("/test/dir/file.py")
        self.handler.on_modified(event)

        mock_reload.assert_called_once_with(self.callback, self.directory)

    @patch("soir.watcher.reload_code")
    def test_on_modified_non_python_file(self, mock_reload: Mock) -> None:
        """Test that modifying non-Python file does not trigger reload."""
        event = events.FileModifiedEvent("/test/dir/file.txt")
        self.handler.on_modified(event)

        mock_reload.assert_not_called()

    @patch("soir.watcher.reload_code")
    def test_on_modified_directory(self, mock_reload: Mock) -> None:
        """Test that directory modification events are ignored."""
        event = events.DirModifiedEvent("/test/dir/subdir")
        self.handler.on_modified(event)

        mock_reload.assert_not_called()

    @patch("soir.watcher.reload_code")
    def test_on_created_python_file(self, mock_reload: Mock) -> None:
        """Test that creating a Python file triggers reload."""
        event = events.FileCreatedEvent("/test/dir/new_file.py")
        self.handler.on_created(event)

        mock_reload.assert_called_once_with(self.callback, self.directory)

    @patch("soir.watcher.reload_code")
    def test_on_created_non_python_file(self, mock_reload: Mock) -> None:
        """Test that creating non-Python file does not trigger reload."""
        event = events.FileCreatedEvent("/test/dir/new_file.js")
        self.handler.on_created(event)

        mock_reload.assert_not_called()

    @patch("soir.watcher.reload_code")
    def test_on_created_directory(self, mock_reload: Mock) -> None:
        """Test that directory creation events are ignored."""
        event = events.DirCreatedEvent("/test/dir/new_subdir")
        self.handler.on_created(event)

        mock_reload.assert_not_called()


class TestWatcher(unittest.TestCase):
    """Test cases for Watcher class."""

    def setUp(self) -> None:
        """Set up test fixtures."""
        self.config = Config(
            dsp=Config.DspConfig(),
            live=Config.LiveConfig(directory="/test/live"),
        )
        self.callback = Mock()

    @patch("soir.watcher.observers.Observer")
    def test_watcher_initialization(self, mock_observer_class: Mock) -> None:
        """Test that Watcher initializes correctly."""
        mock_observer = Mock()
        mock_observer_class.return_value = mock_observer

        Watcher(self.config, self.callback)

        # Verify observer was created
        mock_observer_class.assert_called_once()

        # Verify schedule was called with correct parameters
        mock_observer.schedule.assert_called_once()
        call_args = mock_observer.schedule.call_args
        handler = call_args[0][0]
        self.assertIsInstance(handler, LiveCodeUpdateHandler)
        self.assertEqual(call_args[1]["path"], "/test/live")
        self.assertTrue(call_args[1]["recursive"])

    @patch("soir.watcher.reload_code")
    @patch("soir.watcher.observers.Observer")
    def test_watcher_start(self, mock_observer_class: Mock, mock_reload: Mock) -> None:
        """Test that start() performs initial load and starts observer."""
        mock_observer = Mock()
        mock_observer_class.return_value = mock_observer

        watcher = Watcher(self.config, self.callback)
        watcher.start()

        # Verify initial reload was called
        mock_reload.assert_called_once_with(self.callback, "/test/live")

        # Verify observer was started
        mock_observer.start.assert_called_once()

    @patch("soir.watcher.observers.Observer")
    def test_watcher_stop(self, mock_observer_class: Mock) -> None:
        """Test that stop() stops and joins observer."""
        mock_observer = Mock()
        mock_observer_class.return_value = mock_observer

        watcher = Watcher(self.config, self.callback)
        watcher.stop()

        # Verify observer was stopped and joined
        mock_observer.stop.assert_called_once()
        mock_observer.join.assert_called_once()

    @patch("soir.watcher.observers.Observer")
    def test_watcher_uses_config_directory(self, mock_observer_class: Mock) -> None:
        """Test that Watcher uses directory from config."""
        mock_observer = Mock()
        mock_observer_class.return_value = mock_observer

        custom_config = Config(
            dsp=Config.DspConfig(),
            live=Config.LiveConfig(directory="/custom/path"),
        )
        Watcher(custom_config, self.callback)

        # Verify the custom directory was used
        call_args = mock_observer.schedule.call_args
        handler = call_args[0][0]
        self.assertEqual(handler.directory, "/custom/path")
        self.assertEqual(call_args[1]["path"], "/custom/path")


if __name__ == "__main__":
    unittest.main()
