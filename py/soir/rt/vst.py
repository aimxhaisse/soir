"""VST3 plugin management.

This module provides functions to list and interact with VST3 plugins.
Plugins are automatically scanned when the engine starts.
"""

from typing import cast

from soir._bindings import rt as _rt

from soir.rt import errors


def plugins() -> list[dict[str, str]]:
    """Get a list of available VST3 plugins.

    @public

    Returns a list of dictionaries with plugin information:

    - uid: unique identifier for the plugin
    - name: display name of the plugin
    - vendor: plugin vendor/manufacturer
    - category: plugin category (e.g., "Fx|EQ")
    - path: path to the plugin bundle

    Returns:
        List of plugin information dictionaries.
    """
    return cast(list[dict[str, str]], _rt.vst_get_plugins_())


def require(name: str) -> None:
    """Checks if a VST exists on the system.

    @public

    Raises:
        VSTNotFoundException: raised when the VST does not exist.
    """
    for p in plugins():
        if p.get("name") == name:
            return

    raise errors.VSTNotFoundException


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
