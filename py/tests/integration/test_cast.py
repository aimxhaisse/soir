"""Integration tests for the CastServer."""

import http.client
import json
import socket
import time
from typing import Any, ClassVar

from soir.cast import CastServer
from soir.config import Config

from .base import SoirIntegrationTestCase

_CAST_PORT = 15002


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


class TestCastServer(SoirIntegrationTestCase):
    """Tests for the CastServer HTTP server."""

    config_overrides: ClassVar[dict[str, Any]] = {"initial_bpm": 120}

    def setUp(self) -> None:
        super().setUp()
        cfg = Config(
            dsp=Config.DspConfig(),
            live=Config.LiveConfig(),
            cast=Config.CastConfig(enabled=True, port=_CAST_PORT),
        )
        self.cast_server = CastServer(cfg)
        self.cast_server.start()
        self.assertTrue(
            _wait_for_port("127.0.0.1", _CAST_PORT),
            f"CastServer did not open port {_CAST_PORT} in time",
        )

    def tearDown(self) -> None:
        self.cast_server.stop()
        super().tearDown()

    def test_index_returns_html(self) -> None:
        """GET / returns 200 with HTML content."""
        conn = http.client.HTTPConnection("127.0.0.1", _CAST_PORT, timeout=5)
        conn.request("GET", "/")
        response = conn.getresponse()
        body = response.read()
        conn.close()

        self.assertEqual(response.status, 200)
        self.assertIn(b"<!DOCTYPE html>", body)

    def test_state_returns_valid_json(self) -> None:
        """GET /state returns 200 with a valid JSON snapshot."""
        conn = http.client.HTTPConnection("127.0.0.1", _CAST_PORT, timeout=5)
        conn.request("GET", "/state")
        response = conn.getresponse()
        body = response.read()
        conn.close()

        self.assertEqual(response.status, 200)
        self.assertEqual(response.getheader("Content-Type"), "application/json")

        data = json.loads(body)
        self.assertIn("bpm", data)
        self.assertIn("beat", data)
        self.assertIn("tracks", data)
        self.assertIn("master_levels", data)
        self.assertIsInstance(data["bpm"], float | int)
        self.assertIsInstance(data["beat"], float | int)
        self.assertIsInstance(data["tracks"], list)
        self.assertIn("peak_left", data["master_levels"])
        self.assertIn("peak_right", data["master_levels"])

    def test_state_bpm_matches_config(self) -> None:
        """Snapshot BPM matches the initial_bpm set in the engine config."""
        conn = http.client.HTTPConnection("127.0.0.1", _CAST_PORT, timeout=5)
        conn.request("GET", "/state")
        response = conn.getresponse()
        data = json.loads(response.read())
        conn.close()

        self.assertAlmostEqual(data["bpm"], 120.0, places=0)

    def test_events_returns_sse_stream(self) -> None:
        """GET /events returns text/event-stream with at least one data frame."""
        conn = http.client.HTTPConnection("127.0.0.1", _CAST_PORT, timeout=5)
        conn.request("GET", "/events")
        response = conn.getresponse()

        self.assertEqual(response.status, 200)
        self.assertIn("text/event-stream", response.getheader("Content-Type", ""))

        # Read until we get one complete SSE frame (ends with \n\n).
        buf = b""
        deadline = time.monotonic() + 5.0
        while b"\n\n" not in buf and time.monotonic() < deadline:
            chunk = response.read(256)
            if not chunk:
                break
            buf += chunk

        conn.close()

        self.assertIn(b"data: ", buf, "Expected at least one SSE data frame")
        frame = buf.split(b"data: ", 1)[1].split(b"\n\n")[0]
        data = json.loads(frame)
        self.assertIn("bpm", data)
        self.assertIn("beat", data)
