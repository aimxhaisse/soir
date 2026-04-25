"""Console-script entry point for the `soir` command.

Re-execs under `-Xgil=0` if the GIL is still enabled, then hands off to
`soir.app:app`. Some third-party imports (e.g. watchdog) flip the GIL
back on at import time unless the flag is set.
"""

import os
import sys
import sysconfig

import typer

from soir.app import app


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


def main() -> None:
    _require_free_threaded()
    _reexec_without_gil()
    app()


if __name__ == "__main__":
    main()
