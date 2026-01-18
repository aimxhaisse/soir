"""Sample packs management module for Soir.

This module provides utilities for listing and managing Soir sample packs.
"""

import glob
import json
import os
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
from dataclasses import dataclass

import typer

from soir.config import get_soir_dir

app = typer.Typer(help="Sample packs management commands", no_args_is_help=True)


@dataclass
class Pack:
    """Sample pack metadata.

    Args:
        name: Pack name
        description: Pack description
        author: Pack author
    """

    name: str
    description: str
    author: str


def load_available_packs() -> dict[str, Pack]:
    """Load the sample packs registry from SOIR_DIR/lib/samples/registry.json.

    Returns:
        Dictionary mapping pack names to Pack objects

    Raises:
        ConfigurationError: If SOIR_DIR is not set
    """
    soir_dir = get_soir_dir()
    registry_path = os.path.join(soir_dir, "lib", "samples", "registry.json")
    if not os.path.exists(registry_path):
        return {}

    try:
        with open(registry_path, "r") as f:
            registry = json.load(f)
            packs = {}
            for pack in registry.get("packs", []):
                packs[pack["name"]] = Pack(
                    name=pack["name"],
                    description=pack["description"],
                    author=pack["author"],
                )
            return packs
    except Exception:
        return {}


def get_installed_packs() -> dict[str, Pack]:
    """Get installed packs from the samples directory.

    Returns:
        Dictionary mapping pack names to Pack objects

    Raises:
        ConfigurationError: If SOIR_DIR is not set
    """
    soir_dir = get_soir_dir()
    samples_dir = os.path.join(soir_dir, "lib", "samples")
    if not os.path.exists(samples_dir):
        return {}

    result = {}
    for f in glob.glob(os.path.join(samples_dir, "*.pack.json")):
        with open(f) as fh:
            pack = json.load(fh)
            result[pack["name"]] = Pack(
                name=pack["name"],
                description=pack.get("description", "?"),
                author=pack.get("author", "?"),
            )
    return result


@app.command("ls")
def list_packs() -> None:
    """List all available sample packs.

    Shows which packs are installed (in registry) and which are local/remote.
    """
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
    for char in "_()[]{};,.!? ":
        name = name.replace(char, "-")
    return name.lower()


def is_ffmpeg_installed() -> bool:
    """Check if ffmpeg and ffprobe are installed.

    Returns:
        True if both ffmpeg and ffprobe are installed, False otherwise
    """
    try:
        ffmpeg_result = subprocess.run(
            ["ffmpeg", "-version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        ffprobe_result = subprocess.run(
            ["ffprobe", "-version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
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
                    raise ValueError(
                        f"Collision in sample names: {name} already exists"
                    )
                names.add(name)
                samples.append({"name": name, "path": full_path})
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
            [
                "ffprobe",
                "-v",
                "error",
                "-show_entries",
                "stream=sample_rate,channels",
                "-of",
                "default=noprint_wrappers=1:nokey=1",
                file_path,
            ],
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            return True, 0, 0

        lines = result.stdout.strip().split("\n")
        if len(lines) >= 2:
            sample_rate = int(float(lines[0]))
            channels = int(lines[1])

            needs_conversion = sample_rate != 48000 or not file_path.endswith(".wav")

            return needs_conversion, sample_rate, channels
        else:
            return True, 0, 0
    except Exception:
        return True, 0, 0


def convert_audio_to_wav(file_path: str) -> None:
    """Convert audio file to WAV 48kHz format, preserving channels.

    Args:
        file_path: The path to the audio file
    """
    output_temp = f"{file_path}.temp.wav"
    try:
        needs_conversion, sample_rate, channels = check_audio_properties(file_path)

        if not needs_conversion:
            typer.echo("Already at 48kHz, skipping conversion")
            return

        typer.echo(f"Converting from {sample_rate}Hz to 48kHz")

        result = subprocess.run(
            ["ffmpeg", "-i", file_path, "-ar", "48000", output_temp],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
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


def remove_pack_files(pack_name: str, samples_dir: str) -> None:
    """Remove pack directory and metadata file.

    Args:
        pack_name: Name of the pack to remove
        samples_dir: Path to the samples directory
    """
    pack_dir = os.path.join(samples_dir, pack_name)
    pack_json = os.path.join(samples_dir, f"{pack_name}.pack.json")

    if os.path.exists(pack_dir):
        shutil.rmtree(pack_dir)
    if os.path.exists(pack_json):
        os.remove(pack_json)


def get_pack_name_from_tarball(tarball_path: str) -> str:
    """Get pack name from a tarball by finding the .pack.json file.

    Args:
        tarball_path: Path to the tarball file

    Returns:
        The pack name

    Raises:
        ValueError: If tarball doesn't contain a valid pack
    """
    with tarfile.open(tarball_path, "r:gz") as tar:
        for name in tar.getnames():
            if name.endswith(".pack.json"):
                return name.replace(".pack.json", "")
    raise ValueError("Tarball does not contain a .pack.json file")


def extract_pack_from_tarball(tarball_path: str, samples_dir: str) -> str:
    """Extract a pack tarball to the samples directory and return pack name.

    Args:
        tarball_path: Path to the tarball file
        samples_dir: Path to the samples directory

    Returns:
        The name of the extracted pack

    Raises:
        ValueError: If tarball doesn't contain a valid pack
    """
    pack_name = get_pack_name_from_tarball(tarball_path)
    with tarfile.open(tarball_path, "r:gz") as tar:
        tar.extractall(path=samples_dir)
    return pack_name


@app.command("mk")
def create_pack(
    input_dir: str = typer.Argument(..., help="Directory with samples to pack"),
    name: str = typer.Option(
        None, help="Name for the sample pack (defaults to directory name)"
    ),
    description: str = typer.Option(
        "Sample pack created with Soir", help="Description of the sample pack"
    ),
    author: str = typer.Option("Anonymous", help="Author of the sample pack"),
    override: bool = typer.Option(
        False, "--override", help="Override existing pack with same name"
    ),
    bundle: bool = typer.Option(
        False, "--bundle", help="Create a tarball of the sample pack"
    ),
) -> None:
    """Create a sample pack from an input directory.

    Converts samples to the correct format and organizes them into a pack.
    """
    soir_dir = get_soir_dir()

    if not os.path.isdir(input_dir):
        typer.echo(f"Error: Input directory '{input_dir}' does not exist")
        raise typer.Exit(1)

    pack_name = name if name else os.path.basename(os.path.abspath(input_dir))

    samples_dir = os.path.join(soir_dir, "lib", "samples")
    output_dir = os.path.join(samples_dir, pack_name)

    if os.path.abspath(input_dir) == os.path.abspath(output_dir):
        typer.echo("Error: Input directory cannot be the same as output directory")
        raise typer.Exit(1)

    if os.path.exists(output_dir) and not override:
        typer.echo(
            f"Error: Sample pack '{pack_name}' already exists. "
            "Use --override to replace it."
        )
        raise typer.Exit(1)

    if not is_ffmpeg_installed():
        typer.echo(
            "Error: ffmpeg and/or ffprobe are not installed. "
            "Please install them first."
        )
        raise typer.Exit(1)

    try:
        typer.echo("Scanning for audio files...")
        samples = get_samples_from_directory(input_dir)

        if not samples:
            typer.echo("No audio files found in the input directory")
            raise typer.Exit(1)

        typer.echo(f"Found {len(samples)} audio files")

        if not os.path.exists(samples_dir):
            os.makedirs(samples_dir)

        if os.path.exists(output_dir):
            typer.echo(f"Removing existing pack '{pack_name}'...")
            shutil.rmtree(output_dir)

        os.makedirs(output_dir)

        typer.echo("Copying and converting samples...")
        processed_samples = []

        for sample in samples:
            src_path = sample["path"]
            dest_filename = f"{sample['name']}.wav"
            dest_path = os.path.join(output_dir, dest_filename)

            shutil.copy2(src_path, dest_path)

            typer.echo(f"Processing {dest_filename}...")
            convert_audio_to_wav(dest_path)

            processed_samples.append(
                {"name": sample["name"], "path": os.path.join(pack_name, dest_filename)}
            )

        pack_data = {
            "name": pack_name,
            "description": description,
            "author": author,
            "samples": processed_samples,
        }

        json_path = os.path.join(samples_dir, f"{pack_name}.pack.json")
        with open(json_path, "w") as f:
            json.dump(pack_data, f, indent=2)

        typer.echo(f"Sample pack '{pack_name}' created successfully")

        if bundle:
            typer.echo("Creating tarball...")
            tarball_path = os.path.join(
                os.path.dirname(samples_dir), f"{pack_name}.tar.gz"
            )

            with tarfile.open(tarball_path, "w:gz") as tar:
                tar.add(json_path, arcname=os.path.basename(json_path))
                tar.add(output_dir, arcname=os.path.basename(output_dir))

            typer.echo(f"Tarball created at {tarball_path}")

    except Exception as e:
        typer.echo(f"Error creating sample pack: {str(e)}")
        raise typer.Exit(1)


def _is_local_tarball(source: str) -> bool:
    """Check if source is a local tarball file."""
    return os.path.isfile(source) and (
        source.endswith(".tar.gz") or source.endswith(".tgz")
    )


def _get_registry_url(pack_name: str, registry_path: str) -> str | None:
    """Get download URL for a pack from the registry."""
    try:
        with open(registry_path, "r") as f:
            registry = json.load(f)
            for pack in registry.get("packs", []):
                if pack["name"] == pack_name:
                    url = pack.get("url")
                    return str(url) if url else None
    except Exception:
        pass
    return None


@app.command("install")
def install_pack(
    source: str = typer.Argument(
        ..., help="Pack name from registry or path to local .tar.gz file"
    ),
    force: bool = typer.Option(
        False, "--force", "-f", help="Force reinstallation if already installed"
    ),
) -> None:
    """Install a sample pack from the registry or a local file.

    Args:
        source: Pack name from registry or path to local tarball
        force: Force reinstallation even if already installed
    """
    soir_dir = get_soir_dir()
    samples_dir = os.path.join(soir_dir, "lib", "samples")
    if not os.path.exists(samples_dir):
        os.makedirs(samples_dir)

    installed_packs = get_installed_packs()
    is_local = _is_local_tarball(source)
    pack_url = ""

    if is_local:
        try:
            pack_name = get_pack_name_from_tarball(source)
        except (tarfile.TarError, ValueError) as e:
            typer.echo(f"Error reading tarball: {str(e)}")
            raise typer.Exit(1)
        tarball_path = source
    else:
        pack_name = source
        available_packs = load_available_packs()
        if pack_name not in available_packs:
            typer.echo(f"Error: Sample pack '{pack_name}' not found in registry.")
            raise typer.Exit(1)

        registry_path = os.path.join(samples_dir, "registry.json")
        pack_url = _get_registry_url(pack_name, registry_path) or ""
        if not pack_url:
            typer.echo(f"Error: URL for '{pack_name}' not found in registry.")
            raise typer.Exit(1)

    if pack_name in installed_packs and not force:
        typer.echo(
            f"Sample pack '{pack_name}' is already installed. "
            "Use --force to reinstall."
        )
        return

    if pack_name in installed_packs:
        typer.echo(f"Removing existing installation of '{pack_name}'...")
        remove_pack_files(pack_name, samples_dir)

    try:
        if is_local:
            typer.echo(f"Installing '{pack_name}' from local file...")
        else:
            typer.echo(f"Downloading '{pack_name}'...")
            with tempfile.NamedTemporaryFile(
                suffix=".tar.gz", delete=False
            ) as temp_file:
                tarball_path = temp_file.name
            urllib.request.urlretrieve(pack_url, tarball_path)

        typer.echo(f"Extracting '{pack_name}'...")
        extract_pack_from_tarball(tarball_path, samples_dir)

        if not is_local:
            os.remove(tarball_path)

        typer.echo(f"Sample pack '{pack_name}' installed successfully.")
    except Exception as e:
        typer.echo(f"Error installing sample pack: {str(e)}")
        raise typer.Exit(1)


@app.command("rm")
def remove_pack(
    pack_name: str = typer.Argument(..., help="Name of the sample pack to remove")
) -> None:
    """Remove an installed sample pack.

    Args:
        pack_name: Name of the sample pack to remove
    """
    soir_dir = get_soir_dir()
    samples_dir = os.path.join(soir_dir, "lib", "samples")
    installed_packs = get_installed_packs()

    if pack_name not in installed_packs:
        typer.echo(f"Error: Sample pack '{pack_name}' is not installed.")
        raise typer.Exit(1)

    try:
        remove_pack_files(pack_name, samples_dir)
        typer.echo(f"Sample pack '{pack_name}' removed successfully.")
    except Exception as e:
        typer.echo(f"Error removing sample pack: {str(e)}")
        raise typer.Exit(1)


@app.command("info")
def pack_info(
    pack_name: str = typer.Argument(
        ..., help="Name of the sample pack to display info for"
    )
) -> None:
    """Display detailed information about a pack.

    Args:
        pack_name: Name of the sample pack
    """
    soir_dir = get_soir_dir()

    samples_dir = os.path.join(soir_dir, "lib", "samples")
    installed_packs = get_installed_packs()
    available_packs = load_available_packs()

    if pack_name not in installed_packs and pack_name not in available_packs:
        typer.echo(f"Error: Sample pack '{pack_name}' not found.")
        raise typer.Exit(1)

    is_installed = pack_name in installed_packs
    is_local = pack_name not in available_packs

    typer.echo(f"Pack: {pack_name}")

    if is_installed:
        pack_json = os.path.join(samples_dir, f"{pack_name}.pack.json")
        with open(pack_json, "r") as f:
            pack_data = json.load(f)

        typer.echo(f"Description: {pack_data.get('description', '?')}")
        typer.echo(f"Author: {pack_data.get('author', '?')}")
        typer.echo("Status: Installed")
        typer.echo(f"Location: {'Local' if is_local else 'Remote'}")

        samples = pack_data.get("samples", [])
        typer.echo(f"Samples: {len(samples)} files")

        pack_dir = os.path.join(samples_dir, pack_name)
        if os.path.exists(pack_dir):
            total_size = 0
            for root, _, files in os.walk(pack_dir):
                for file in files:
                    total_size += os.path.getsize(os.path.join(root, file))

            size_mb = total_size / (1024 * 1024)
            typer.echo(f"Total Size: {size_mb:.1f} MB")

        if samples:
            typer.echo("\nSamples:")
            for sample in samples:
                typer.echo(f"  - {sample['name']}.wav")
    else:
        pack = available_packs[pack_name]
        typer.echo(f"Description: {pack.description}")
        typer.echo(f"Author: {pack.author}")
        typer.echo("Status: Not installed")
        typer.echo("Location: Remote")
