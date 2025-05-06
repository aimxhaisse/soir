"""
Documentation management module for Soir.

This module provides utilities for building and serving Soir documentation.
"""

import sys
import typer
from mkdocs.__main__ import cli as mkdocs_cli

from .utils import (
    expand_env_vars,
)


app = typer.Typer(help="Documentation management commands", no_args_is_help=True)


@app.command("serve")
def serve_docs():
    """
    Serve the Soir documentation locally.
    
    This builds the documentation and starts a local server to view it.
    """
    mkdocs_cli([
        'serve',
        '--verbose',
        '--config-file', expand_env_vars('$SOIR_DIR/www/mkdocs.yml'),
        '--dev-addr', 'localhost:8192'
    ])


@app.command("build")
def build_docs():
    """
    Build the Soir documentation.
    
    This builds the documentation without starting a server.
    """
    mkdocs_cli([
        'build',
        '--site-dir', 'site/',
        '--config-file', 'www/mkdocs.yml'
    ])
