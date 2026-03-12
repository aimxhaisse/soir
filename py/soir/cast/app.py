"""CastServer — embedded session web server for Soir.

Serves a minimal WebGL visualizer, an SSE stream of engine state, and a
WebSocket endpoint that streams raw PCM audio to the browser.
"""

import time
from collections.abc import Generator
from pathlib import Path
from threading import Event, Thread

from flask import Flask, Response, render_template
from flask_sock import Sock
from werkzeug.serving import BaseWSGIServer, make_server

from soir import _bindings
from soir.config import Config


def _make_flask_app(config: Config, stop_event: Event) -> Flask:
    app = Flask(__name__, template_folder=str(Path(__file__).parent / "templates"))
    sock = Sock(app)

    @app.route("/")
    def index() -> str:
        return render_template("index.html")

    @app.route("/state")
    def state() -> Response:
        return Response(
            _bindings.state.get_snapshot_(),
            mimetype="application/json",
        )

    @app.route("/events")
    def events() -> Response:
        def stream() -> Generator[str]:
            last = None
            while not stop_event.is_set():
                current = _bindings.state.get_snapshot_()
                if current != last:
                    yield f"data: {current}\n\n"
                    last = current
                time.sleep(0.05)

        return Response(stream(), mimetype="text/event-stream")

    @sock.route("/pcm")
    def pcm(ws: object) -> None:  # type: ignore[type-arg]
        offset = _bindings.pcm.get_current_offset_()
        while not stop_event.is_set():
            data, offset = _bindings.pcm.read_(offset)
            if data:
                ws.send(data)  # type: ignore[attr-defined]

    return app


class CastServer:
    """Embedded HTTP server that streams engine state to a browser visualizer."""

    def __init__(self, config: Config) -> None:
        self._config = config
        self._server: BaseWSGIServer | None = None
        self._thread: Thread | None = None
        self._stop_event = Event()

    def start(self) -> None:
        app = _make_flask_app(self._config, self._stop_event)
        self._server = make_server("0.0.0.0", self._config.cast.port, app, threaded=True)
        self._thread = Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop_event.set()
        if self._server:
            self._server.shutdown()
        if self._thread:
            self._thread.join()
