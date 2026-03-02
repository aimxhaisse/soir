"""Integration tests for the audio HTTP streaming server."""

import http.client
import socket
import time
from typing import Any, ClassVar

from .base import SoirIntegrationTestCase

# Use a dedicated port to avoid conflicts with the default (5001).
_STREAMING_PORT = 15001


def _wait_for_port(host: str, port: int, timeout: float = 10.0) -> bool:
    """Return True once the port accepts TCP connections, False on timeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return True
        except ConnectionRefusedError, OSError:
            time.sleep(0.05)
    return False


class TestStreaming(SoirIntegrationTestCase):
    """Tests for the Ogg/Opus HTTP streaming server."""

    config_overrides: ClassVar[dict[str, Any]] = {
        "enable_streaming": True,
        "streaming_port": _STREAMING_PORT,
    }

    def test_server_starts(self) -> None:
        """HTTP server binds and accepts TCP connections when enable_streaming is true."""
        self.assertTrue(
            _wait_for_port("127.0.0.1", _STREAMING_PORT),
            f"Streaming server did not open port {_STREAMING_PORT} in time",
        )

    def test_ogg_stream_header(self) -> None:
        """Streaming endpoint returns valid Ogg/Opus data starting with OggS."""
        self.assertTrue(_wait_for_port("127.0.0.1", _STREAMING_PORT))

        conn = http.client.HTTPConnection("127.0.0.1", _STREAMING_PORT, timeout=5)
        conn.request("GET", "/stream.opus")
        response = conn.getresponse()

        self.assertEqual(response.status, 200)
        self.assertEqual(response.getheader("Content-Type"), "audio/ogg")

        # Read enough bytes to cover the Ogg capture pattern (first 4 bytes = "OggS").
        data = response.read(64)
        conn.close()

        self.assertEqual(
            data[:4],
            b"OggS",
            f"Expected Ogg magic bytes 'OggS', got {data[:4]!r}",
        )

    def test_unknown_path_returns_404(self) -> None:
        """Non-streaming paths return 404."""
        self.assertTrue(_wait_for_port("127.0.0.1", _STREAMING_PORT))

        conn = http.client.HTTPConnection("127.0.0.1", _STREAMING_PORT, timeout=5)
        conn.request("GET", "/not-a-stream")
        response = conn.getresponse()
        conn.close()

        self.assertEqual(response.status, 404)
