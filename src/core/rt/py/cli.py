"""
Soir CLI application.

This module provides the main CLI interface for Soir.
"""

import typer
from typing import Optional

from soir import session


app = typer.Typer(help="Soir: Python Sings Alive")


app.add_typer(session.app, name="session", help="Manage Soir sessions")


app()
