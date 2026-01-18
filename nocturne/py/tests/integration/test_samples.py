"""Integration tests for sample pack management."""

import io
import json
import os
import shutil
import sys
import tarfile
import tempfile
import unittest
import wave
from pathlib import Path

import click.exceptions

import soir.cli.samples
from soir.cli.samples import (
    create_pack,
    get_installed_packs,
    get_samples_from_directory,
    install_pack,
    list_packs,
    normalize_name,
    pack_info,
    remove_pack,
)


def create_test_audio_file(
    path: str, sample_rate: int = 44100, duration: float = 0.1
) -> None:
    """Create a test WAV file.

    Args:
        path: Path to create the WAV file
        sample_rate: Sample rate in Hz
        duration: Duration in seconds
    """
    os.makedirs(os.path.dirname(path), exist_ok=True)

    with wave.open(path, "w") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)

        num_frames = int(sample_rate * duration)
        wav_file.writeframes(b"\x00\x00" * num_frames)


def create_test_registry(packs: list[dict], registry_path: str) -> None:
    """Create a test registry.json file.

    Args:
        packs: List of pack dictionaries
        registry_path: Path to create the registry file
    """
    os.makedirs(os.path.dirname(registry_path), exist_ok=True)
    with open(registry_path, "w") as f:
        json.dump({"packs": packs}, f, indent=2)


def verify_pack_structure(
    pack_name: str, expected_samples: list[str], samples_dir: str
) -> None:
    """Verify pack was created correctly.

    Args:
        pack_name: Name of the pack
        expected_samples: List of expected sample names (without .wav extension)
        samples_dir: Path to samples directory

    Raises:
        AssertionError: If pack structure is invalid
    """
    pack_json = os.path.join(samples_dir, f"{pack_name}.pack.json")
    if not os.path.exists(pack_json):
        raise AssertionError(f"Pack JSON {pack_json} does not exist")

    with open(pack_json) as f:
        pack_data = json.load(f)

    if pack_data["name"] != pack_name:
        raise AssertionError(f"Expected pack name {pack_name}, got {pack_data['name']}")
    if len(pack_data["samples"]) != len(expected_samples):
        raise AssertionError(
            f"Expected {len(expected_samples)} samples, "
            f"got {len(pack_data['samples'])}"
        )

    sample_names = {s["name"] for s in pack_data["samples"]}
    expected_set = set(expected_samples)
    if sample_names != expected_set:
        raise AssertionError(f"Expected samples {expected_set}, got {sample_names}")

    pack_dir = os.path.join(samples_dir, pack_name)
    if not os.path.exists(pack_dir):
        raise AssertionError(f"Pack directory {pack_dir} does not exist")

    for sample_name in expected_samples:
        sample_path = os.path.join(pack_dir, f"{sample_name}.wav")
        if not os.path.exists(sample_path):
            raise AssertionError(f"Sample {sample_path} does not exist")


class SamplesTestBase(unittest.TestCase):
    """Base class for samples tests with common setup."""

    def setUp(self) -> None:
        """Set up test environment."""
        self.tmpdir = tempfile.mkdtemp()
        self.soir_dir = Path(self.tmpdir) / "soir"
        self.soir_dir.mkdir()

        self.samples_dir = self.soir_dir / "lib" / "samples"
        self.samples_dir.mkdir(parents=True)

        registry = self.samples_dir / "registry.json"
        registry.write_text('{"packs": []}')

        os.environ["SOIR_DIR"] = str(self.soir_dir)
        soir.cli.samples.SOIR_DIR = str(self.soir_dir)

    def tearDown(self) -> None:
        """Clean up test environment."""
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def create_sample_audio_dir(self) -> str:
        """Create a directory with test audio files.

        Returns:
            Path to directory with test audio files
        """
        audio_dir = Path(self.soir_dir) / "test_audio"
        audio_dir.mkdir()

        create_test_audio_file(str(audio_dir / "kick.wav"), 44100)
        create_test_audio_file(str(audio_dir / "snare.wav"), 48000)
        create_test_audio_file(str(audio_dir / "hihat.wav"), 44100)

        return str(audio_dir)


class SamplesLsTest(SamplesTestBase):
    """Tests for samples ls command."""

    def test_samples_ls_empty(self) -> None:
        """Test listing packs when no packs installed."""
        captured_output = io.StringIO()
        sys.stdout = captured_output
        try:
            list_packs()
            output = captured_output.getvalue()
            self.assertIn("PACK NAME", output)
            self.assertIn("STATUS", output)
            self.assertIn("LOCATION", output)
        finally:
            sys.stdout = sys.__stdout__

    def test_samples_ls_with_installed(self) -> None:
        """Test listing packs with installed packs."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="test-pack",
            description="Test pack",
            author="Test Author",
            override=False,
            bundle=False,
        )

        captured_output = io.StringIO()
        sys.stdout = captured_output
        try:
            list_packs()
            output = captured_output.getvalue()
            self.assertIn("test-pack", output)
            self.assertIn("✓ Installed", output)
            self.assertIn("Local", output)
        finally:
            sys.stdout = sys.__stdout__


class SamplesMkTest(SamplesTestBase):
    """Tests for samples mk command."""

    def test_samples_mk_basic(self) -> None:
        """Test creating a basic sample pack."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="basic-pack",
            description="Basic test pack",
            author="Test Author",
            override=False,
            bundle=False,
        )

        verify_pack_structure(
            "basic-pack", ["kick", "snare", "hihat"], str(self.samples_dir)
        )

    def test_samples_mk_with_bundle(self) -> None:
        """Test creating a pack with --bundle flag."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="bundled-pack",
            description="Bundled test pack",
            author="Test Author",
            override=False,
            bundle=True,
        )

        tarball_path = os.path.join(self.soir_dir, "lib", "bundled-pack.tar.gz")
        self.assertTrue(os.path.exists(tarball_path))

        with tarfile.open(tarball_path, "r:gz") as tar:
            members = tar.getnames()
            self.assertIn("bundled-pack.pack.json", members)
            self.assertIn("bundled-pack", members)

    def test_samples_mk_name_collision(self) -> None:
        """Test that creating a pack with duplicate sample names fails."""
        audio_dir = Path(self.soir_dir) / "collision_test"
        audio_dir.mkdir()

        create_test_audio_file(str(audio_dir / "kick_01.wav"))
        create_test_audio_file(str(audio_dir / "kick-01.wav"))

        with self.assertRaises(ValueError) as ctx:
            get_samples_from_directory(str(audio_dir))

        self.assertIn("Collision in sample names", str(ctx.exception))

    def test_samples_mk_override(self) -> None:
        """Test creating a pack with --override flag."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="override-test",
            description="First version",
            author="Author 1",
            override=False,
            bundle=False,
        )

        second_audio_dir = Path(self.soir_dir) / "second_audio"
        second_audio_dir.mkdir()
        create_test_audio_file(str(second_audio_dir / "bass.wav"))

        create_pack(
            input_dir=str(second_audio_dir),
            name="override-test",
            description="Second version",
            author="Author 2",
            override=True,
            bundle=False,
        )

        pack_json = os.path.join(self.samples_dir, "override-test.pack.json")
        with open(pack_json) as f:
            pack_data = json.load(f)

        self.assertEqual(pack_data["description"], "Second version")
        self.assertEqual(pack_data["author"], "Author 2")
        self.assertEqual(len(pack_data["samples"]), 1)

    def test_samples_mk_without_override_fails(self) -> None:
        """Test that creating a pack without --override fails if it exists."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="existing-pack",
            description="Existing pack",
            author="Author",
            override=False,
            bundle=False,
        )

        with self.assertRaises(click.exceptions.Exit):
            create_pack(
                input_dir=sample_audio_dir,
                name="existing-pack",
                description="Try to recreate",
                author="Author",
                override=False,
                bundle=False,
            )


class SamplesRmTest(SamplesTestBase):
    """Tests for samples rm command."""

    def test_samples_rm(self) -> None:
        """Test removing an installed pack."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="remove-me",
            description="Pack to remove",
            author="Test Author",
            override=False,
            bundle=False,
        )

        pack_dir = os.path.join(self.samples_dir, "remove-me")
        pack_json = os.path.join(self.samples_dir, "remove-me.pack.json")

        self.assertTrue(os.path.exists(pack_dir))
        self.assertTrue(os.path.exists(pack_json))

        remove_pack("remove-me")

        self.assertFalse(os.path.exists(pack_dir))
        self.assertFalse(os.path.exists(pack_json))

    def test_samples_rm_not_installed(self) -> None:
        """Test removing a pack that is not installed fails."""
        with self.assertRaises(click.exceptions.Exit):
            remove_pack("nonexistent-pack")


class SamplesInfoTest(SamplesTestBase):
    """Tests for samples info command."""

    def test_samples_info_installed(self) -> None:
        """Test info command for installed pack."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="info-pack",
            description="Info test pack",
            author="Info Author",
            override=False,
            bundle=False,
        )

        captured_output = io.StringIO()
        sys.stdout = captured_output
        try:
            pack_info("info-pack")
            output = captured_output.getvalue()

            self.assertIn("Pack: info-pack", output)
            self.assertIn("Description: Info test pack", output)
            self.assertIn("Author: Info Author", output)
            self.assertIn("Status: Installed", output)
            self.assertIn("Location: Local", output)
            self.assertIn("Samples: 3 files", output)
        finally:
            sys.stdout = sys.__stdout__

    def test_samples_info_not_found(self) -> None:
        """Test info command for nonexistent pack."""
        with self.assertRaises(click.exceptions.Exit):
            pack_info("nonexistent-pack")


class SamplesInstallTest(SamplesTestBase):
    """Tests for samples install command."""

    def test_samples_install_from_registry(self) -> None:
        """Test installing a pack from the registry."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="remote-pack",
            description="Remote pack",
            author="Remote Author",
            override=False,
            bundle=True,
        )

        tarball_path = os.path.join(self.soir_dir, "lib", "remote-pack.tar.gz")

        registry_data = [
            {
                "name": "remote-pack",
                "description": "Remote pack",
                "author": "Remote Author",
                "url": f"file://{tarball_path}",
            }
        ]
        registry_path = os.path.join(self.samples_dir, "registry.json")
        create_test_registry(registry_data, registry_path)

        pack_dir = os.path.join(self.samples_dir, "remote-pack")
        pack_json = os.path.join(self.samples_dir, "remote-pack.pack.json")

        if os.path.exists(pack_dir):
            shutil.rmtree(pack_dir)
        if os.path.exists(pack_json):
            os.remove(pack_json)

        install_pack("remote-pack", force=False)

        self.assertTrue(os.path.exists(pack_dir))
        self.assertTrue(os.path.exists(pack_json))

    def test_samples_install_already_installed(self) -> None:
        """Test installing a pack that is already installed."""
        sample_audio_dir = self.create_sample_audio_dir()

        create_pack(
            input_dir=sample_audio_dir,
            name="already-installed",
            description="Already installed pack",
            author="Author",
            override=False,
            bundle=False,
        )

        registry_data = [
            {
                "name": "already-installed",
                "description": "Already installed pack",
                "author": "Author",
                "url": "https://example.com/pack.tar.gz",
            }
        ]
        registry_path = os.path.join(self.samples_dir, "registry.json")
        create_test_registry(registry_data, registry_path)

        captured_output = io.StringIO()
        sys.stdout = captured_output
        try:
            install_pack("already-installed", force=False)
            output = captured_output.getvalue()
            self.assertIn("already installed", output.lower())
        finally:
            sys.stdout = sys.__stdout__


class NormalizeNameTest(unittest.TestCase):
    """Tests for normalize_name function."""

    def test_normalize_name(self) -> None:
        """Test name normalization."""
        self.assertEqual(normalize_name("Kick_001"), "kick-001")
        self.assertEqual(normalize_name("Snare (2)"), "snare--2-")
        self.assertEqual(normalize_name("HiHat.wav"), "hihat-wav")
        self.assertEqual(normalize_name("Bass_Line_01"), "bass-line-01")
