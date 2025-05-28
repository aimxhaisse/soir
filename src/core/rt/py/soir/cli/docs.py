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
from soir.system import (
    set_force_kill_at_shutdown_,
)

app = typer.Typer(help="Documentation management commands", no_args_is_help=True)


@app.command("serve")
def serve_docs():
    """
    Serve the Soir documentation locally.
    
    This builds the documentation and starts a local server to view it.
    """
    # This is required to kill the serve command upon ctrl+c,
    # otherwise it would hang indefinitely.
    set_force_kill_at_shutdown_(True)

    mkdocs_cli([
        'serve',
        '--verbose',
        '--config-file', expand_env_vars('$SOIR_DIR/www/mkdocs.yml'),
        '--dev-addr', 'localhost:8192'
    ])


@app.command("mk")
def build_docs():
    """
    Build the Soir documentation.
    
    This builds the documentation without starting a server.
    """
    mkdocs_cli([
        'build',
        '--site-dir', 'site/',
        '--config-file', expand_env_vars('$SOIR_DIR/www/mkdocs.yml'),
    ])
