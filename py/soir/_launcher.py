"""Console-script entry point for the `soir` command.

Re-execs under `-Xgil=0` if the GIL is still enabled, then hands off to
`soir.app:app`. Some third-party imports (e.g. watchdog) flip the GIL
back on at import time unless the flag is set.
"""

import os
import sys
import sysconfig
from typing import Any

import typer

try:
    from soir.app import app as _app

    _import_error: ImportError | None = None
except ImportError as e:
    _app: Any = None  # type: ignore[no-redef]
    _import_error = e


def _require_free_threaded() -> None:
    if not sysconfig.get_config_var("Py_GIL_DISABLED"):
        typer.echo(
            "Soir requires free-threaded Python 3.14t.\n"
            "Install it with:  uv python install 3.14.2t",
            err=True,
        )
        raise typer.Exit(1)


def _reexec_without_gil() -> None:
    if os.environ.get("_SOIR_GIL_REEXEC") == "1":
        return
    if not sys._is_gil_enabled():
        return
    os.environ["_SOIR_GIL_REEXEC"] = "1"
    os.execv(
        sys.executable,
        [sys.executable, "-Xgil=0", "-m", "soir.app", *sys.argv[1:]],
    )


def _check_native_dependencies() -> None:
    if _import_error is None:
        return
    msg = str(_import_error)
    if "cannot open shared object file" in msg:
        lib = msg.split(":", 1)[0].strip()
        typer.echo(
            f"Soir failed to load a required system library: {lib}\n\n"
            "Install the JACK audio runtime via your package manager:\n"
            "  Debian/Ubuntu:   sudo apt install libjack-jackd2-0\n"
            "  Fedora/RHEL:     sudo dnf install jack-audio-connection-kit\n"
            "  Arch:            sudo pacman -S jack2  (or pipewire-jack)",
            err=True,
        )
    else:
        typer.echo(f"Failed to import Soir engine: {msg}", err=True)
    raise typer.Exit(1)


def main() -> None:
    _require_free_threaded()
    _reexec_without_gil()
    _check_native_dependencies()
    _app()


if __name__ == "__main__":
    main()
