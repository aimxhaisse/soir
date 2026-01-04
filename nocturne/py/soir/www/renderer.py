"""Documentation rendering module.

This module handles rendering parsed documentation into HTML,
including syntax highlighting and markdown conversion.
"""

from typing import Any, cast

from docstring_parser import Docstring
from markdown import Markdown
from pygments import highlight  # type: ignore[import-untyped]
from pygments.formatters import HtmlFormatter  # type: ignore[import-untyped]
from pygments.lexers import PythonLexer  # type: ignore[import-untyped]

from .docs import MemberDoc, ModuleDoc


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


def render_member(member: MemberDoc, md: Markdown) -> dict[str, Any]:
    """Render a single member (function, class, etc.) to HTML.

    Args:
        member: MemberDoc containing member documentation.
        md: Markdown instance for rendering.

    Returns:
        Dictionary with rendered HTML fields.
    """
    member_rendered = {
        "name": member.name,
        "type": member.type,
        "description_html": render_description(member.parsed, md),
        "params": member.parsed.params,
        "returns": member.parsed.returns,
        "raises": member.parsed.raises,
        "signature_html": None,
        "methods": [],
    }

    # Highlight signature if present
    if member.signature:
        member_rendered["signature_html"] = highlight_signature(
            member.name, member.signature
        )

    # Render methods if this is a class
    methods_list = cast(list[dict[str, Any]], member_rendered["methods"])
    if member.type == "class" and member.methods:
        for method in member.methods:
            method_rendered = {
                "name": method.name,
                "description_html": render_description(method.parsed, md),
                "params": method.parsed.params,
                "returns": method.parsed.returns,
                "raises": method.parsed.raises,
                "signature_html": None,
            }

            # Highlight method signature if present
            if method.signature:
                method_rendered["signature_html"] = highlight_signature(
                    method.name, method.signature
                )

            methods_list.append(method_rendered)

    return member_rendered


def render_module(module_data: ModuleDoc, md: Markdown) -> dict[str, Any]:
    """Render a complete module's documentation to HTML.

    Args:
        module_data: ModuleDoc containing module documentation.
        md: Markdown instance for rendering.

    Returns:
        Dictionary with rendered HTML fields.
    """
    module_doc_html = render_description(module_data.parsed, md)

    members_rendered = []
    for member in module_data.members:
        members_rendered.append(render_member(member, md))

    return {
        "name": module_data.name,
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
