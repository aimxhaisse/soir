"""VST3 plugin management.

@public

This module provides functions to list and interact with VST3 plugins.
Plugins are automatically scanned when the engine starts.
"""

from typing import cast

from soir._bindings import rt as _rt


def plugins() -> list[dict[str, str]]:
    """Get a list of available VST3 plugins.

    @public

    Returns a list of dictionaries with plugin information:
    - uid: Unique identifier for the plugin
    - name: Display name of the plugin
    - vendor: Plugin vendor/manufacturer
    - category: Plugin category (e.g., "Fx|EQ")
    - path: Path to the plugin bundle

    Returns:
        List of plugin information dictionaries.
    """
    return cast(list[dict[str, str]], _rt.vst_get_plugins_())


def instruments() -> list[dict[str, str]]:
    """Get a list of available VST3 instrument plugins.

    @public

    Returns only plugins with type "instrument".

    Returns:
        List of instrument plugin information dictionaries.
    """
    return [p for p in plugins() if p.get("type") == "instrument"]


def effects() -> list[dict[str, str]]:
    """Get a list of available VST3 effect plugins.

    @public

    Returns only plugins with type "effect".

    Returns:
        List of effect plugin information dictionaries.
    """
    return [p for p in plugins() if p.get("type") == "effect"]
