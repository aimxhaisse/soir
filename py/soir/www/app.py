"""Flask web application for serving Soir documentation."""

import threading
import time
from pathlib import Path
from typing import Any

from flask import Flask, abort, render_template

from .docs import DocsCache, extract_module_docs
from .renderer import create_markdown_renderer, render_module

PUBLIC_MODULES = [
    "bpm",
    "ctrls",
    "errors",
    "fx",
    "levels",
    "midi",
    "rnd",
    "sampler",
    "system",
    "tracks",
]


def mk_docs(app: Flask, cache: DocsCache) -> None:
    app.logger.info("Generating API documentation...")
    for module_name in PUBLIC_MODULES:
        docs = extract_module_docs(f"soir.rt.{module_name}")
        cache.set(module_name, docs)
        app.logger.info(f"  ✓ Generated docs for {module_name}")
    app.logger.info(f"Documentation generated for {len(cache)} modules")


def create_app() -> Flask:
    app = Flask(__name__)

    @app.template_filter("truncate_toc")
    def truncate_toc(text: str, max_total: int = 14) -> str:
        if len(text) <= max_total:
            return text
        return text[: max_total - 3] + "..."

    app.extensions["markdown"] = create_markdown_renderer()

    docs_cache = DocsCache()
    mk_docs(app, docs_cache)
    app.extensions["docs_cache"] = docs_cache

    @app.route("/")
    def home() -> str:
        """Render the home page."""
        content = render_markdown("home.md")
        return render_template("page.html", content=content, title="Home")

    @app.route("/quickstart")
    def quickstart() -> str:
        """Render the quickstart page."""
        content = render_markdown("quickstart.md")
        return render_template("page.html", content=content, title="Quickstart")

    @app.route("/examples")
    def examples() -> str:
        """Render the examples page."""
        content = render_markdown("examples.md")
        return render_template("page.html", content=content, title="Examples")

    @app.route("/reference")
    def reference() -> str:
        """Render the reference index page."""
        return render_template(
            "reference_index.html", title="Reference", modules=PUBLIC_MODULES
        )

    @app.route("/reference/core")
    def reference_core() -> str:
        """Render the core facilities reference page."""
        rt_data = extract_module_docs("soir.rt")
        md = app.extensions["markdown"]
        module_rendered = render_module(rt_data, md)

        return render_template(
            "reference.html",
            title="Core Facilities - Reference",
            modules=PUBLIC_MODULES,
            doc=module_rendered["doc"],
            members=module_rendered["members"],
        )

    @app.route("/reference/<path:module_path>")
    def reference_module(module_path: str) -> str:
        """Render a specific module's reference documentation.

        Args:
            module_path: Module name (e.g., 'bpm', 'sampler').

        Returns:
            Rendered HTML for the module documentation.
        """
        cache = app.extensions["docs_cache"]
        if not cache.has(module_path):
            abort(404)

        module_data = cache.get(module_path)
        if module_data is None:
            abort(404)

        md = app.extensions["markdown"]
        module_rendered = render_module(module_data, md)

        return render_template(
            "reference_module.html",
            title=f"{module_path} - Reference",
            module=module_rendered,
        )

    @app.errorhandler(404)
    def not_found(error: Exception) -> tuple[str, int]:
        return (
            render_template(
                "page.html",
                content="<h1>404 Not Found</h1><p>The requested page does not exist.</p>",
                title="Not Found",
            ),
            404,
        )

    @app.errorhandler(500)
    def server_error(error: Exception) -> tuple[str, int]:
        return (
            render_template(
                "page.html",
                content="<h1>500 Internal Server Error</h1><p>Something went wrong.</p>",
                title="Error",
            ),
            500,
        )

    def render_markdown(filename: str) -> Any:
        content_dir = Path(__file__).parent / "content"
        file_path = content_dir / filename

        if not file_path.exists():
            return "<p>Content coming soon.</p>"

        with open(file_path, "r", encoding="utf-8") as f:
            markdown_text = f.read()

        return app.extensions["markdown"].convert(markdown_text)

    return app


def start_server(
    port: int = 5000,
    host: str = "127.0.0.1",
    dev: bool = False,
    open_browser: bool = True,
) -> None:
    """Start the Flask development server.

    Args:
        port: Port number to run the server on.
        host: Host address to bind to. Use "0.0.0.0" for containers.
        dev: Enable development mode with debug and auto-reload.
        open_browser: Automatically open the browser to the app URL.
    """
    app = create_app()

    url = f"http://{host}:{port}"

    if open_browser and host == "127.0.0.1":

        def open_in_browser() -> None:
            time.sleep(0.5)

        threading.Thread(target=open_in_browser, daemon=True).start()

    print(f"Starting Soir documentation server at {url}")
    print("Press Ctrl+C to stop the server")

    app.run(host=host, port=port, debug=dev)
