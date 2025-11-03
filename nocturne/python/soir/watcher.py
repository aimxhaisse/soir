"""Code watcher for Soir.

This modules defines a file system watcher that monitors a directory for
changes to code files. When a change is detected, it triggers a callback
to handle the update.

We update everything everytime, this is because the Soir RT has to
handle cleanups whenever some code is deleted. It can't know what was
deleted if we only send partial diffs.
"""

from typing import Callable
from watchdog import events, observers
import os

from soir.config import Config


# This sends the updated code to the Soir runtime engine. The full
# code of the entire live directory is expected.
CodeUpdateCallback = Callable[[str], None]


def reload_code(cb: CodeUpdateCallback, directory: str) -> None:
    """Reloads the entire codebase from the given directory."""
    files: list[str] = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".py"):
                files.append(os.path.join(root, file))

    contents: str = ""
    for file in sorted(files):
        with open(file, "r") as f:
            contents += f.read()

    cb(contents)


class LiveCodeUpdateHandler(events.FileSystemEventHandler):
    """Handler for live code updates."""

    def __init__(self, directory: str, cb: CodeUpdateCallback):
        super().__init__()
        self.directory = directory
        self.cb = cb

    def on_modified(self, event: events.FileSystemEvent) -> None:
        """Called when a file is modified."""
        if not event.is_directory and str(event.src_path).endswith(".py"):
            reload_code(self.cb, self.directory)

    def on_created(self, event: events.FileSystemEvent) -> None:
        """Called when a file is created."""
        if not event.is_directory and str(event.src_path).endswith(".py"):
            reload_code(self.cb, self.directory)


class Watcher:
    """File system watcher for code changes."""

    def __init__(self, cfg: Config, cb: CodeUpdateCallback):
        self.cfg = cfg
        self.cb = cb

        self.watchdog = observers.Observer()
        self.watchdog.schedule(
            LiveCodeUpdateHandler(self.cfg.live.directory, self.cb),
            path=self.cfg.live.directory,
            recursive=True,
        )

    def start(self) -> None:
        """Start the watcher & initial load of code files."""
        reload_code(self.cb, self.cfg.live.directory)
        self.watchdog.start()

    def stop(self) -> None:
        """Stop the watcher."""
        self.watchdog.stop()
        self.watchdog.join()
