"""
Sample packs management module for Soir.

This module provides utilities for listing and managing Soir sample packs.
"""

from dataclasses import dataclass
import os
import json
import glob
import typer
import yaml

from .utils import (
    expand_env_vars,
)

app = typer.Typer(help="Sample packs management commands", no_args_is_help=True)

SOIR_DIR = os.getenv("SOIR_DIR")


@dataclass
class Pack:
    name: str
    description: str
    author: str


def load_available_packs() -> dict[str, Pack]:
    """Load the sample packs registry from SOIR_DIR/samples/registry.json"""
    registry_path = os.path.join(SOIR_DIR, "samples", "registry.json")
    if not os.path.exists(registry_path):
        return {}

    try:
        with open(registry_path, "r") as f:
            registry = json.load(f)
            packs = {}
            for pack in registry.get('packs', []):
                packs[pack['name']] = Pack(
                    name=pack['name'], description=pack['description'], author=pack['author'])
            return packs
    except Exception:
        return {}


def get_installed_packs() -> dict[str, Pack]:
    """Get installed packs from the samples directory"""
    samples_dir = os.path.join(SOIR_DIR, "samples")
    if not os.path.exists(samples_dir):
        return {}

    result = {}
    for f in glob.glob(os.path.join(samples_dir, "*.pack.yaml")):
        with open(f) as fh:
            pack = yaml.safe_load(fh)
            result[pack['name']] = Pack(
                name=pack['name'], description='?', author='?')
    return result


@app.command("ls")
def list_packs():
    """
    List all available sample packs.
    
    Shows which packs are installed (in registry) and which are local/remote.
    """
    if not SOIR_DIR:
        typer.echo("SOIR_DIR environment variable not set.")
        raise typer.Exit(1)
        
    available_packs = load_available_packs()
    installed_packs = get_installed_packs()

    typer.echo(f"{'PACK NAME':<30} {'STATUS':<15} {'LOCATION':<10}")
    typer.echo(f"{'-' * 30:<30} {'-' * 15:<15} {'-' * 10:<10}")
    
    all_pack_names = set(list(available_packs.keys()) + list(installed_packs.keys()))
    
    for pack_name in sorted(all_pack_names):
        is_installed = pack_name in installed_packs
        is_local = pack_name in available_packs
        status = "✓ Installed" if is_installed else "✗ Not installed"
        location = "Local" if is_local else "Remote"
        typer.echo(f"{pack_name:<30} {status:<15} {location:<10}")
