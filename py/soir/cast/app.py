"""CastServer — embedded session web server for Soir.

Serves a minimal WebGL visualizer and an SSE stream of engine state.
"""

import time
from collections.abc import Generator
from pathlib import Path
from threading import Thread

from flask import Flask, Response, render_template
from werkzeug.serving import BaseWSGIServer, make_server

from soir import _bindings
from soir.config import Config


def _make_flask_app(config: Config) -> Flask:
    app = Flask(__name__, template_folder=str(Path(__file__).parent / "templates"))

    streaming_url = (
        f"http://localhost:{config.dsp.streaming_port}/stream.opus"
        if config.dsp.enable_streaming
        else None
    )

    @app.route("/")
    def index() -> str:
        return render_template("index.html", streaming_url=streaming_url)

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
            while True:
                current = _bindings.state.get_snapshot_()
                if current != last:
                    yield f"data: {current}\n\n"
                    last = current
                time.sleep(0.05)

        return Response(stream(), mimetype="text/event-stream")

    return app


class CastServer:
    """Embedded HTTP server that streams engine state to a browser visualizer."""

    def __init__(self, config: Config) -> None:
        self._config = config
        self._server: BaseWSGIServer | None = None
        self._thread: Thread | None = None

    def start(self) -> None:
        app = _make_flask_app(self._config)
        self._server = make_server("0.0.0.0", self._config.cast.port, app)
        self._thread = Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        if self._server:
            self._server.shutdown()
        if self._thread:
            self._thread.join()
