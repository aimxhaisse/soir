import json

from soir.rt._ctrls import (
    Control_,
)


def serialize_parameters(params: dict[str, object] | None) -> str:
    """Helper to serialize parameters to JSON.

    This is meant to pass parameters to arbitrary DSP code via JSON
    encoding. The controls are assumed to be strings.
    """
    result: dict[str, str | float | int | None] = {}
    if params:
        for k, v in params.items():
            if isinstance(v, Control_):
                result[k] = v.name_
            else:
                result[k] = v  # type: ignore[assignment]
    return json.dumps(result)
