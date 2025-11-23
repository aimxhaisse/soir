"""Documentation rendering module.

This module handles rendering parsed documentation into HTML,
including syntax highlighting and markdown conversion.
"""

from typing import Any

from docstring_parser import Docstring
from markdown import Markdown
from pygments import highlight  # type: ignore[import-untyped]
from pygments.formatters import HtmlFormatter  # type: ignore[import-untyped]
from pygments.lexers import PythonLexer  # type: ignore[import-untyped]


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
    result: str = highlight(code, PythonLexer(), formatter)
    return result


def render_description(parsed: Docstring, md: Markdown) -> str:
    """Render a docstring's description (short + long) to HTML.

    Args:
        parsed: Parsed docstring object from docstring_parser.
        md: Markdown instance for rendering.

    Returns:
        HTML string of the rendered description.
    """
    full_desc = ""
    if parsed.short_description:
        full_desc = parsed.short_description
    if parsed.long_description:
        if full_desc:
            full_desc += "\n\n" + parsed.long_description
        else:
            full_desc = parsed.long_description

    return md.convert(full_desc) if full_desc else ""


def render_member(member: dict[str, Any], md: Markdown) -> dict[str, Any]:
    """Render a single member (function, class, etc.) to HTML.

    Args:
        member: Dictionary containing member data with keys:
                - name: Member name
                - type: Member type (function, class, decorator, etc.)
                - signature: Function signature string (if callable)
                - parsed: Parsed docstring object
        md: Markdown instance for rendering.

    Returns:
        Dictionary with rendered HTML fields:
        - name: Member name
        - type: Member type
        - description_html: Rendered description
        - params: List of parameter documentation
        - returns: Return value documentation
        - raises: List of raised exceptions
        - signature_html: Syntax-highlighted signature (if present)
    """
    parsed = member["parsed"]

    member_rendered = {
        "name": member["name"],
        "type": member["type"],
        "description_html": render_description(parsed, md),
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

    return member_rendered


def render_module(module_data: dict[str, Any], md: Markdown) -> dict[str, Any]:
    """Render a complete module's documentation to HTML.

    Args:
        module_data: Dictionary containing module data from extract_module_docs:
                     - name: Module name
                     - parsed: Parsed module docstring
                     - members: List of member dictionaries
        md: Markdown instance for rendering.

    Returns:
        Dictionary with rendered HTML fields:
        - name: Module name
        - doc: Rendered module description
        - members: List of rendered members
    """
    module_doc_html = render_description(module_data["parsed"], md)

    members_rendered = []
    for member in module_data["members"]:
        members_rendered.append(render_member(member, md))

    return {
        "name": module_data["name"],
        "doc": module_doc_html,
        "members": members_rendered,
    }


def create_markdown_renderer() -> Markdown:
    """Create a configured Markdown renderer for documentation.

    Returns:
        Configured Markdown instance with code highlighting support.
    """
    return Markdown(
        extensions=[
            "codehilite",
            "tables",
            "fenced_code",
        ],
        extension_configs={
            "codehilite": {
                "css_class": "highlight",
                "guess_lang": False,
                "linenums": False,
            }
        },
    )
