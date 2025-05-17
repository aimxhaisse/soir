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
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

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
        is_local = pack_name not in available_packs
        status = "✓ Installed" if is_installed else "✗ Not installed"
        location = "Local" if is_local else "Remote"
        typer.echo(f"{pack_name:<30} {status:<15} {location:<10}")


def normalize_name(name: str) -> str:
    """Normalize a name to be used as a sample name.
    
    Args:
        name: The name to normalize
        
    Returns:
        The normalized name
    """
    for char in ["_", "(", ")", "[", "]", "{", "}", ":", ";", ",", ".", "!", "?", " "]:
        name = name.replace(char, "-")
    return name.lower()


def is_ffmpeg_installed() -> bool:
    """Check if ffmpeg and ffprobe are installed.
    
    Returns:
        True if both ffmpeg and ffprobe are installed, False otherwise
    """
    try:
        # Check ffmpeg
        ffmpeg_result = subprocess.run(
            ["ffmpeg", "-version"],
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.DEVNULL
        )
        
        # Check ffprobe
        ffprobe_result = subprocess.run(
            ["ffprobe", "-version"],
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.DEVNULL
        )
        
        return ffmpeg_result.returncode == 0 and ffprobe_result.returncode == 0
    except FileNotFoundError:
        return False


def get_samples_from_directory(directory: str) -> list[dict[str, str]]:
    """Get all samples from a directory.
    
    Args:
        directory: The directory to search for samples
        
    Returns:
        A list of samples with name and path
        
    Raises:
        ValueError: If there's a collision in sample names
    """
    samples = []
    names = set()
    for root, _, files in os.walk(directory):
        for file in files:
            if file.lower().endswith((".wav", ".mp3", ".ogg", ".flac")):
                full_path = os.path.join(root, file)
                name = normalize_name(os.path.splitext(file)[0])
                if name in names:
                    raise ValueError(f"Collision in sample names: {name} already exists")
                names.add(name)
                samples.append({'name': name, 'path': full_path})
    return samples


def check_audio_properties(file_path: str) -> tuple[bool, int, int]:
    """Check if audio file is already in the correct format.
    
    Args:
        file_path: The path to the audio file
        
    Returns:
        Tuple of (needs_conversion, sample_rate, channels)
    """
    try:
        result = subprocess.run(
            ["ffprobe", "-v", "error", "-show_entries", "stream=sample_rate,channels", 
             "-of", "default=noprint_wrappers=1:nokey=1", file_path],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            # If we can't determine, assume we need to convert
            return True, 0, 0
            
        lines = result.stdout.strip().split('\n')
        if len(lines) >= 2:
            sample_rate = int(float(lines[0]))
            channels = int(lines[1])
            
            # Check if conversion is needed
            needs_conversion = sample_rate != 48000 or not file_path.endswith(".wav")
            
            return needs_conversion, sample_rate, channels
        else:
            return True, 0, 0
    except Exception:
        # If any error occurs, assume we need to convert
        return True, 0, 0


def convert_audio_to_wav(file_path: str) -> None:
    """Convert audio file to WAV 48kHz format, preserving channels.
    
    Args:
        file_path: The path to the audio file
    """
    output_temp = f"{file_path}.temp.wav"
    try:
        # Check current properties
        needs_conversion, sample_rate, channels = check_audio_properties(file_path)
        
        if not needs_conversion:
            typer.echo(f"Already at 48kHz, skipping conversion")
            return
            
        # Preserve the original number of channels, but convert to 48kHz
        typer.echo(f"Converting from {sample_rate}Hz to 48kHz")
        
        result = subprocess.run(
            ["ffmpeg", "-i", file_path, "-ar", "48000", output_temp],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        
        if result.returncode == 0:
            os.remove(file_path)
            os.rename(output_temp, file_path)
        else:
            if os.path.exists(output_temp):
                os.remove(output_temp)
            typer.echo(f"Warning: Failed to convert {file_path}")
    except Exception as e:
        if os.path.exists(output_temp):
            os.remove(output_temp)
        typer.echo(f"Error converting {file_path}: {str(e)}")


@app.command("mk")
def create_pack(
    input_dir: str = typer.Argument(..., help="Directory containing samples to pack"),
    name: str = typer.Option(None, help="Name for the sample pack (defaults to directory name)"),
    description: str = typer.Option("Sample pack created with Soir", help="Description of the sample pack"),
    author: str = typer.Option("Anonymous", help="Author of the sample pack"),
    override: bool = typer.Option(False, "--override", help="Override existing pack with same name"),
    bundle: bool = typer.Option(False, "--bundle", help="Create a tarball of the sample pack")
):
    """
    Create a sample pack from an input directory.
    
    Converts samples to the correct format and organizes them into a pack.
    """
    if not SOIR_DIR:
        typer.echo("SOIR_DIR environment variable not set.")
        raise typer.Exit(1)
    
    # Check if input directory exists
    if not os.path.isdir(input_dir):
        typer.echo(f"Error: Input directory '{input_dir}' does not exist")
        raise typer.Exit(1)
    
    # Determine pack name
    pack_name = name if name else os.path.basename(os.path.abspath(input_dir))
    
    # Check if output would be the same as input
    samples_dir = os.path.join(SOIR_DIR, "samples")
    output_dir = os.path.join(samples_dir, pack_name)
    
    if os.path.abspath(input_dir) == os.path.abspath(output_dir):
        typer.echo("Error: Input directory cannot be the same as output directory")
        raise typer.Exit(1)
    
    # Check if pack already exists
    if os.path.exists(output_dir) and not override:
        typer.echo(f"Error: Sample pack '{pack_name}' already exists. Use --override to replace it.")
        raise typer.Exit(1)
    
    # Check if ffmpeg and ffprobe are installed
    if not is_ffmpeg_installed():
        typer.echo("Error: ffmpeg and/or ffprobe are not installed. Please install them first.")
        raise typer.Exit(1)
    
    try:
        # Get all samples
        typer.echo("Scanning for audio files...")
        samples = get_samples_from_directory(input_dir)
        
        if not samples:
            typer.echo("No audio files found in the input directory")
            raise typer.Exit(1)
        
        typer.echo(f"Found {len(samples)} audio files")
        
        # Create output directory
        if not os.path.exists(samples_dir):
            os.makedirs(samples_dir)
        
        if os.path.exists(output_dir):
            typer.echo(f"Removing existing pack '{pack_name}'...")
            shutil.rmtree(output_dir)
        
        os.makedirs(output_dir)
        
        # Copy and convert samples
        typer.echo("Copying and converting samples...")
        processed_samples = []
        
        for sample in samples:
            src_path = sample['path']
            dest_filename = f"{sample['name']}.wav"
            dest_path = os.path.join(output_dir, dest_filename)
            
            # Copy file
            shutil.copy2(src_path, dest_path)
            
            # Check if conversion is needed
            typer.echo(f"Processing {dest_filename}...")
            convert_audio_to_wav(dest_path)
            
            processed_samples.append({
                'name': sample['name'],
                'path': dest_filename
            })
        
        # Create pack YAML file
        pack_data = {
            'name': pack_name,
            'description': description,
            'author': author,
            'samples': processed_samples
        }
        
        yaml_path = os.path.join(samples_dir, f"{pack_name}.pack.yaml")
        with open(yaml_path, "w") as f:
            yaml.dump(pack_data, f)
        
        typer.echo(f"Sample pack '{pack_name}' created successfully")
        
        # Create tarball if requested
        if bundle:
            typer.echo("Creating tarball...")
            tarball_path = os.path.join(os.path.dirname(samples_dir), f"{pack_name}.tar.gz")
            
            with tarfile.open(tarball_path, "w:gz") as tar:
                # Add the YAML file
                tar.add(yaml_path, arcname=os.path.basename(yaml_path))
                
                # Add the samples directory
                tar.add(output_dir, arcname=os.path.basename(output_dir))
            
            typer.echo(f"Tarball created at {tarball_path}")
        
    except Exception as e:
        typer.echo(f"Error creating sample pack: {str(e)}")
        raise typer.Exit(1)


@app.command("install")
def install_pack(
    pack_name: str = typer.Argument(..., help="Name of the sample pack to install"),
    force: bool = typer.Option(False, "--force", "-f", help="Force reinstallation if already installed")
):
    """
    Install a sample pack from the registry.
    
    Checks if the pack is already installed, downloads it from the registry URL if not,
    and extracts it into the SOIR_DIR/samples directory.
    
    Args:
        pack_name: Name of the sample pack to install
        force: Force reinstallation even if already installed
    """
    if not SOIR_DIR:
        typer.echo("SOIR_DIR environment variable not set.")
        raise typer.Exit(1)
        
    samples_dir = os.path.join(SOIR_DIR, "samples")
    if not os.path.exists(samples_dir):
        os.makedirs(samples_dir)
    
    # Check if pack is already installed
    installed_packs = get_installed_packs()
    if pack_name in installed_packs and not force:
        typer.echo(f"Sample pack '{pack_name}' is already installed. Use --force to reinstall.")
        return
    
    # Get pack info from registry
    available_packs = load_available_packs()
    if pack_name not in available_packs:
        typer.echo(f"Error: Sample pack '{pack_name}' not found in registry.")
        raise typer.Exit(1)
    
    # Get the URL from registry.json
    registry_path = os.path.join(SOIR_DIR, "samples", "registry.json")
    try:
        with open(registry_path, "r") as f:
            registry = json.load(f)
            pack_url = None
            for pack in registry.get('packs', []):
                if pack['name'] == pack_name:
                    pack_url = pack.get('url')
                    break
            
            if not pack_url:
                typer.echo(f"Error: URL for sample pack '{pack_name}' not found in registry.")
                raise typer.Exit(1)
    except Exception as e:
        typer.echo(f"Error reading registry file: {str(e)}")
        raise typer.Exit(1)
    
    # Remove existing pack if force reinstall
    if pack_name in installed_packs:
        typer.echo(f"Removing existing installation of '{pack_name}'...")
        pack_dir = os.path.join(samples_dir, pack_name)
        pack_yaml = os.path.join(samples_dir, f"{pack_name}.pack.yaml")
        
        if os.path.exists(pack_dir):
            shutil.rmtree(pack_dir)
        if os.path.exists(pack_yaml):
            os.remove(pack_yaml)
    
    # Download and extract the pack
    try:
        typer.echo(f"Downloading sample pack '{pack_name}'...")
        with tempfile.NamedTemporaryFile(suffix=".tar.gz", delete=False) as temp_file:
            temp_path = temp_file.name
            
        # Download the file
        urllib.request.urlretrieve(pack_url, temp_path)
        
        typer.echo(f"Extracting sample pack '{pack_name}'...")
        with tarfile.open(temp_path, "r:gz") as tar:
            # Extract files to the samples directory
            tar.extractall(path=samples_dir)
        
        # Clean up temp file
        os.remove(temp_path)
        
        typer.echo(f"Sample pack '{pack_name}' has been installed successfully.")
    except Exception as e:
        typer.echo(f"Error installing sample pack: {str(e)}")
        raise typer.Exit(1)
