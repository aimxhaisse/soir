"""Shared CLI utilities for Soir."""

import os
import re


def expand_env_vars(s: str) -> str:
    """Expand environment variables in a string.

    Args:
        s: String potentially containing environment variables
           in ${VAR} or $VAR format

    Returns:
        String with environment variables expanded
    """

    def _replacer(match: re.Match) -> str:
        var_name = match.group(1) or match.group(2)
        return os.environ.get(var_name, "")

    pattern = r"\$\{([^}]*)\}|\$([a-zA-Z0-9_]+)"
    return re.sub(pattern, _replacer, s)
