"""Flask web application for serving Soir documentation."""

import threading
import time
import webbrowser
from pathlib import Path

import mistune
import pdoc.doc
from flask import Flask, render_template, abort
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import get_lexer_by_name


def create_app() -> Flask:
    """Create and configure the Flask application.

    Returns:
        Configured Flask application instance.
    """
    app = Flask(__name__)

    # Store pdoc data in app context
    app.pdoc_modules = {}

    # Configure mistune with Pygments highlighting
    class HighlightRenderer(mistune.HTMLRenderer):
        def block_code(self, code: str, info: str | None = None) -> str:
            if info:
                try:
                    lexer = get_lexer_by_name(info, stripall=True)
                    formatter = HtmlFormatter()
                    return highlight(code, lexer, formatter)
                except Exception:
                    pass
            return f'<pre><code>{mistune.escape(code)}</code></pre>'

    markdown = mistune.create_markdown(renderer=HighlightRenderer())
    app.markdown = markdown

    @app.route('/')
    def home() -> str:
        """Render the home page."""
        content = render_markdown('home.md')
        return render_template('page.html', content=content, title='Home')

    @app.route('/quickstart')
    def quickstart() -> str:
        """Render the quickstart page."""
        content = render_markdown('quickstart.md')
        return render_template('page.html', content=content, title='Quickstart')

    @app.route('/examples')
    def examples() -> str:
        """Render the examples page."""
        content = render_markdown('examples.md')
        return render_template('page.html', content=content, title='Examples')

    @app.route('/about')
    def about() -> str:
        """Render the about page."""
        content = render_markdown('about.md')
        return render_template('page.html', content=content, title='About')

    @app.route('/reference')
    def reference() -> str:
        """Render the reference index page."""
        return render_template('reference.html', title='API Reference', modules=[])

    @app.route('/reference/<path:module_path>')
    def reference_module(module_path: str) -> str:
        """Render a specific module's reference documentation.

        Args:
            module_path: Dot-separated module path (e.g., 'soir.dsp').

        Returns:
            Rendered HTML for the module documentation.
        """
        # TODO: Implement pdoc integration
        abort(404)

    @app.errorhandler(404)
    def not_found(error: Exception) -> tuple[str, int]:
        """Handle 404 errors."""
        return render_template('page.html',
                             content='<h1>404 Not Found</h1><p>The requested page does not exist.</p>',
                             title='Not Found'), 404

    @app.errorhandler(500)
    def server_error(error: Exception) -> tuple[str, int]:
        """Handle 500 errors."""
        return render_template('page.html',
                             content='<h1>500 Internal Server Error</h1><p>Something went wrong.</p>',
                             title='Error'), 500

    def render_markdown(filename: str) -> str:
        """Load and render a markdown file.

        Args:
            filename: Name of the markdown file in the content directory.

        Returns:
            Rendered HTML content.
        """
        content_dir = Path(__file__).parent / 'content'
        file_path = content_dir / filename

        if not file_path.exists():
            return '<p>Content coming soon.</p>'

        with open(file_path, 'r', encoding='utf-8') as f:
            markdown_text = f.read()

        return app.markdown(markdown_text)

    return app


def start_server(port: int = 5000, dev: bool = False, open_browser: bool = True) -> None:
    """Start the Flask development server.

    Args:
        port: Port number to run the server on.
        dev: Enable development mode with debug and auto-reload.
        open_browser: Automatically open the browser to the app URL.
    """
    app = create_app()

    url = f'http://localhost:{port}'

    if open_browser:
        def open_in_browser() -> None:
            time.sleep(0.5)
            webbrowser.open(url)

        threading.Thread(target=open_in_browser, daemon=True).start()

    print(f'Starting Soir documentation server at {url}')
    print('Press Ctrl+C to stop the server')

    app.run(host='127.0.0.1', port=port, debug=dev)
