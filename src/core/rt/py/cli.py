"""
Soir CLI application.

This module provides the main CLI interface for Soir.
"""

import sys
import typer
from typing import Optional

from soir.cli import session
from soir.cli import docs
from soir.cli import samples


app = typer.Typer(
    help="Soir: Python Sings Alive",
    add_completion=False,
    no_args_is_help=True,
)


app.add_typer(session.app, name="session", help="Manage Soir sessions")
app.add_typer(docs.app, name="docs", help="Manage Soir documentation")
app.add_typer(samples.app, name="samples", help="Manage sample packs")


app()
