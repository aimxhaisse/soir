"""Documentation extraction and parsing module.

This module handles extracting documentation from Python modules,
parsing docstrings, and building structured documentation data.
"""

import importlib
import inspect
from typing import Any

from docstring_parser import parse as parse_docstring


def is_public(doc: str) -> bool:
    """Check if a docstring is marked as public.

    Args:
        doc: The docstring to check.

    Returns:
        True if the docstring contains @public marker.
    """
    return "@public" in doc


def clean_doc(doc: str) -> str:
    """Remove @public marker and normalize line wrapping in docstring.

    Args:
        doc: The docstring to clean.

    Returns:
        Cleaned docstring with @public marker removed and normalized wrapping.
    """
    cleaned = doc.replace("@public", "")
    lines = cleaned.split("\n")
    normalized = []
    i = 0

    while i < len(lines):
        line = lines[i]

        if not line.strip():
            normalized.append(line)
            i += 1
            continue

        if line.strip().startswith("```"):
            normalized.append(line)
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                normalized.append(lines[i])
                i += 1
            if i < len(lines):
                normalized.append(lines[i])
                i += 1
            continue

        if line.strip().endswith(":") and len(line.strip().split()) == 1:
            normalized.append(line)
            i += 1
            continue

        if line.strip().startswith("#"):
            normalized.append(line)
            i += 1
            continue

        paragraph = [line]
        base_indent = len(line) - len(line.lstrip())
        i += 1

        while i < len(lines):
            next_line = lines[i]

            if not next_line.strip():
                break

            if next_line.strip().startswith("```"):
                break

            if next_line.strip().endswith(":") and len(next_line.strip().split()) == 1:
                break

            if next_line.strip().startswith("#"):
                break

            next_indent = len(next_line) - len(next_line.lstrip())

            if next_indent >= base_indent:
                paragraph.append(next_line)
                i += 1
            else:
                break

        if len(paragraph) == 1:
            normalized.append(paragraph[0])
        else:
            indent = paragraph[0][: len(paragraph[0]) - len(paragraph[0].lstrip())]
            content = " ".join(line.strip() for line in paragraph if line.strip())
            normalized.append(indent + content)

    return "\n".join(normalized)


def extract_module_docs(
    module_name: str, full_path: str | None = None
) -> dict[str, Any]:
    """Extract documentation from a module.

    Args:
        module_name: Display name of the module (e.g., 'bpm' or 'rt').
        full_path: Full module path to import (e.g., 'soir.rt.bpm').
                   If None, will construct from module_name.

    Returns:
        Dictionary containing module documentation data with keys:
        - name: Display name of the module
        - full_path: Full import path
        - parsed: Parsed module docstring (from docstring_parser)
        - members: List of documented members (functions, classes, etc.)
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
        # Skip private members
        if name.startswith("_"):
            continue

        # Skip imported members - only include members defined in this module
        if hasattr(obj, "__module__") and obj.__module__ != full_path:
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

        # Determine type (use plural forms for proper rendering)
        if inspect.isfunction(obj):
            # Check if it's a decorator based on name
            decorator_names = {"live", "loop"}
            if name in decorator_names:
                member_type = "decorator"
            else:
                member_type = "function"
        elif inspect.isclass(obj):
            member_type = "class"
        else:
            continue

        member_data = {
            "name": name,
            "type": member_type,
            "signature": signature,
            "parsed": parsed,  # Parsed docstring object
        }

        # If it's a class, extract its methods
        if inspect.isclass(obj):
            methods = []
            for method_name, method_obj in inspect.getmembers(obj):
                # Skip private methods and special methods
                if method_name.startswith("_"):
                    continue

                # Get method docstring
                method_doc = inspect.getdoc(method_obj) or ""

                # Only include methods with @public in their docstring
                if not is_public(method_doc):
                    continue

                # Remove the @public marker
                method_doc = clean_doc(method_doc)

                # Parse the docstring
                method_parsed = parse_docstring(method_doc)

                # Get signature for the method
                method_signature = None
                if callable(method_obj):
                    try:
                        sig = inspect.signature(method_obj)
                        method_signature = str(sig)
                    except (ValueError, TypeError):
                        pass

                methods.append(
                    {
                        "name": method_name,
                        "signature": method_signature,
                        "parsed": method_parsed,
                    }
                )

            member_data["methods"] = methods

        members.append(member_data)

    return {
        "name": module_name,
        "full_path": full_path,
        "parsed": module_parsed,  # Parsed module docstring
        "members": members,
    }


class DocsCache:
    """Documentation cache for storing pre-generated module documentation.

    This class provides a simple cache for module documentation to avoid
    re-extracting documentation on every request.
    """

    def __init__(self) -> None:
        """Initialize an empty documentation cache."""
        self._cache: dict[str, dict[str, Any]] = {}

    def get(self, module_name: str) -> dict[str, Any] | None:
        """Get cached documentation for a module.

        Args:
            module_name: Name of the module to retrieve.

        Returns:
            Module documentation dict if cached, None otherwise.
        """
        return self._cache.get(module_name)

    def set(self, module_name: str, docs: dict[str, Any]) -> None:
        """Store documentation for a module in the cache.

        Args:
            module_name: Name of the module.
            docs: Documentation data to cache.
        """
        self._cache[module_name] = docs

    def has(self, module_name: str) -> bool:
        """Check if module documentation is cached.

        Args:
            module_name: Name of the module to check.

        Returns:
            True if the module is in the cache, False otherwise.
        """
        return module_name in self._cache

    def clear(self) -> None:
        """Clear all cached documentation."""
        self._cache.clear()

    def __len__(self) -> int:
        """Get the number of cached modules.

        Returns:
            Number of modules in the cache.
        """
        return len(self._cache)


def generate_all_docs(module_names: list[str], cache: DocsCache) -> None:
    """Generate documentation for all specified modules and cache them.

    Args:
        module_names: List of module names to generate documentation for.
        cache: DocsCache instance to store the generated documentation.
    """
    for module_name in module_names:
        try:
            docs = extract_module_docs(module_name)
            cache.set(module_name, docs)
        except Exception as e:
            # Store error information in cache
            cache.set(
                module_name,
                {
                    "name": module_name,
                    "doc": f"Error generating documentation: {e}",
                    "members": [],
                },
            )
