"""
Soir CLI application.

This module provides the main CLI interface for Soir.
"""

import sys
import typer
from typing import Optional

from soir.cli import session


app = typer.Typer(help="Soir: Python Sings Alive", add_completion=False)


app.add_typer(session.app, name="session", help="Manage Soir sessions")


if len(sys.argv) == 1:
    app(["--help"])
else:
    app()
