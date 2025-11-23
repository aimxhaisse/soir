"""Flask web application for serving Soir documentation."""

import importlib
import inspect
import threading
import time
from pathlib import Path
from typing import Any

import markdown
from docstring_parser import parse as parse_docstring
from flask import Flask, render_template, abort
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import PythonLexer

# Public modules in py/soir/rt that should be documented
PUBLIC_MODULES = [
    "bpm",
    "ctrls",
    "errors",
    "fx",
    "midi",
    "rnd",
    "sampler",
    "system",
    "tracks",
]

# Global cache for generated documentation
docs_cache: dict[str, dict[str, Any]] = {}


def is_public(doc: str) -> bool:
    """Check if a docstring is marked as public.

    Args:
        doc: The docstring to check.

    Returns:
        True if the docstring contains @public marker.
    """
    return "@public" in doc


def highlight_signature(name: str, signature: str) -> str:
    """Highlight a function signature using Pygments.

    Args:
        name: The function/class name.
        signature: The signature string from inspect.signature().

    Returns:
        HTML string with syntax-highlighted signature (span elements).
    """
    code = f"{name}{signature}"
    formatter = HtmlFormatter(nowrap=True)
    return highlight(code, PythonLexer(), formatter)


def clean_doc(doc: str) -> str:
    """Remove @public marker from docstring.

    Args:
        doc: The docstring to clean.

    Returns:
        Cleaned docstring with @public marker removed.
    """
    # Remove @public marker entirely
    cleaned = doc.replace("@public", "")

    # Clean up any resulting empty lines or extra whitespace
    lines = cleaned.split("\n")
    cleaned_lines = [line.rstrip() for line in lines]

    # Remove consecutive empty lines
    result = []
    prev_empty = False
    for line in cleaned_lines:
        if line.strip() == "":
            if not prev_empty:
                result.append("")
            prev_empty = True
        else:
            result.append(line)
            prev_empty = False

    return "\n".join(result).strip()


def extract_module_docs(module_name: str, full_path: str | None = None) -> dict[str, Any]:
    """Extract documentation from a module.

    Args:
        module_name: Display name of the module (e.g., 'bpm' or 'rt').
        full_path: Full module path to import (e.g., 'soir.rt.bpm').
                   If None, will construct from module_name.

    Returns:
        Dictionary containing module documentation data.
    """
    if full_path is None:
        full_path = f"soir.rt.{module_name}"

    module = importlib.import_module(full_path)

    # Get module docstring and parse it
    module_doc_raw = inspect.getdoc(module) or ""
    module_doc_raw = clean_doc(module_doc_raw)
    module_parsed = parse_docstring(module_doc_raw)

    # Extract public members (functions, classes, etc.)
    members = []
    for name, obj in inspect.getmembers(module):
        # Skip private members and imports
        if name.startswith("_"):
            continue

        # Get docstring
        doc = inspect.getdoc(obj) or ""

        # Only include members with @public in their docstring
        if not is_public(doc):
            continue

        # Remove the @public marker from the doc
        doc = clean_doc(doc)

        # Parse the docstring
        parsed = parse_docstring(doc)

        # Get signature for callables
        signature = None
        if callable(obj):
            try:
                sig = inspect.signature(obj)
                signature = str(sig)
            except (ValueError, TypeError):
                pass

        # Determine type
        if inspect.isfunction(obj):
            member_type = "function"
        elif inspect.isclass(obj):
            member_type = "class"
        else:
            member_type = "attribute"

        members.append(
            {
                "name": name,
                "type": member_type,
                "signature": signature,
                "parsed": parsed,  # Parsed docstring object
            }
        )

    return {
        "name": module_name,
        "full_path": full_path,
        "parsed": module_parsed,  # Parsed module docstring
        "members": members,
    }


def generate_docs_cache(app: Flask) -> None:
    """Generate documentation for all public modules and cache them.

    This is called once on application startup to warm the cache.

    Args:
        app: Flask application instance for logging.
    """
    global docs_cache

    app.logger.info("Generating API documentation...")
    for module_name in PUBLIC_MODULES:
        try:
            docs_cache[module_name] = extract_module_docs(module_name)
            app.logger.info(f"  ✓ Generated docs for {module_name}")
        except Exception as e:
            app.logger.error(f"  ✗ Failed to generate docs for {module_name}: {e}")
            docs_cache[module_name] = {
                "name": module_name,
                "doc": f"Error generating documentation: {e}",
                "members": [],
            }

    app.logger.info(f"Documentation generated for {len(docs_cache)} modules")


def create_app() -> Flask:
    """Create and configure the Flask application.

    Returns:
        Configured Flask application instance.
    """
    app = Flask(__name__)

    app.extensions["markdown"] = markdown.Markdown(
        extensions=[
            "codehilite",
            "tables",
            "fenced_code",
            "nl2br",
        ],
        extension_configs={
            "codehilite": {
                "css_class": "highlight",
                "guess_lang": False,
                "linenums": False,
            }
        },
    )

    # Generate documentation cache on startup
    generate_docs_cache(app)

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
        # Extract documentation from soir.rt.__init__.py using same logic
        rt_data = extract_module_docs("rt", full_path="soir.rt")

        # Render markdown in module description
        md = app.extensions["markdown"]
        parsed_module = rt_data["parsed"]

        # Combine short and long description
        full_desc = ""
        if parsed_module.short_description:
            full_desc = parsed_module.short_description
        if parsed_module.long_description:
            if full_desc:
                full_desc += "\n\n" + parsed_module.long_description
            else:
                full_desc = parsed_module.long_description

        rt_doc_html = md.convert(full_desc) if full_desc else ""

        members_rendered = []
        for member in rt_data["members"]:
            parsed = member["parsed"]

            # Combine short and long description
            desc = ""
            if parsed.short_description:
                desc = parsed.short_description
            if parsed.long_description:
                if desc:
                    desc += "\n\n" + parsed.long_description
                else:
                    desc = parsed.long_description

            member_rendered = {
                "name": member["name"],
                "type": member["type"],
                "description_html": md.convert(desc) if desc else "",
                "params": parsed.params,
                "returns": parsed.returns,
                "raises": parsed.raises,
                "signature_html": None,
            }

            # Highlight signature if present
            if member["signature"]:
                member_rendered["signature_html"] = highlight_signature(
                    member["name"], member["signature"]
                )

            members_rendered.append(member_rendered)

        return render_template(
            "reference.html",
            title="Reference",
            modules=PUBLIC_MODULES,
            doc=rt_doc_html,
            members=members_rendered,
        )

    @app.route("/reference/<path:module_path>")
    def reference_module(module_path: str) -> str:
        """Render a specific module's reference documentation.

        Args:
            module_path: Module name (e.g., 'bpm', 'sampler').

        Returns:
            Rendered HTML for the module documentation.
        """
        if module_path not in docs_cache:
            abort(404)

        module_data = docs_cache[module_path]

        # Render markdown in module description
        md = app.extensions["markdown"]
        parsed_module = module_data["parsed"]

        # Combine short and long description
        full_desc = ""
        if parsed_module.short_description:
            full_desc = parsed_module.short_description
        if parsed_module.long_description:
            if full_desc:
                full_desc += "\n\n" + parsed_module.long_description
            else:
                full_desc = parsed_module.long_description

        module_doc_html = md.convert(full_desc) if full_desc else ""

        members_rendered = []
        for member in module_data["members"]:
            parsed = member["parsed"]

            # Combine short and long description
            desc = ""
            if parsed.short_description:
                desc = parsed.short_description
            if parsed.long_description:
                if desc:
                    desc += "\n\n" + parsed.long_description
                else:
                    desc = parsed.long_description

            member_rendered = {
                "name": member["name"],
                "type": member["type"],
                "description_html": md.convert(desc) if desc else "",
                "params": parsed.params,
                "returns": parsed.returns,
                "raises": parsed.raises,
                "signature_html": None,
            }

            # Highlight signature if present
            if member["signature"]:
                member_rendered["signature_html"] = highlight_signature(
                    member["name"], member["signature"]
                )

            members_rendered.append(member_rendered)

        return render_template(
            "reference_module.html",
            title=f"{module_path} - Reference",
            module={
                "name": module_data["name"],
                "doc": module_doc_html,
                "members": members_rendered,
            },
        )

    @app.errorhandler(404)
    def not_found(error: Exception) -> tuple[str, int]:
        """Handle 404 errors."""
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
        """Handle 500 errors."""
        return (
            render_template(
                "page.html",
                content="<h1>500 Internal Server Error</h1><p>Something went wrong.</p>",
                title="Error",
            ),
            500,
        )

    def render_markdown(filename: str) -> Any:
        """Load and render a markdown file.

        Args:
            filename: Name of the markdown file in the content directory.

        Returns:
            Rendered HTML content.
        """
        content_dir = Path(__file__).parent / "content"
        file_path = content_dir / filename

        if not file_path.exists():
            return "<p>Content coming soon.</p>"

        with open(file_path, "r", encoding="utf-8") as f:
            markdown_text = f.read()

        return app.extensions["markdown"].convert(markdown_text)

    return app


def start_server(
    port: int = 5000, dev: bool = False, open_browser: bool = True
) -> None:
    """Start the Flask development server.

    Args:
        port: Port number to run the server on.
        dev: Enable development mode with debug and auto-reload.
        open_browser: Automatically open the browser to the app URL.
    """
    app = create_app()

    url = f"http://localhost:{port}"

    if open_browser:

        def open_in_browser() -> None:
            time.sleep(0.5)

        threading.Thread(target=open_in_browser, daemon=True).start()

    print(f"Starting Soir documentation server at {url}")
    print("Press Ctrl+C to stop the server")

    app.run(host="127.0.0.1", port=port, debug=dev)
