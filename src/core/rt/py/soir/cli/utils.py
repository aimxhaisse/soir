import os
import re


def expand_env_vars(path: str) -> str:
    """Expand environment variables in a string path."""

    def _replacer(match):
        var_name = match.group(1) or match.group(2)
        return os.environ.get(var_name, "")

    pattern = r"\$\{([^}]*)\}|\$([a-zA-Z0-9_]+)"
    return re.sub(pattern, _replacer, path)
