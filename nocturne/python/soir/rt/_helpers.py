import json

from soir.rt._ctrls import (
    Control_,
)


def serialize_parameters(params: object) -> str:
    """Helper to serialize parameters to JSON.

    This is meant to pass parameters to arbitrary DSP code via JSON
    encoding. The controls are assumed to be strings.
    """
    result = {}
    if params:
        for k, v in params.items():
            if isinstance(v, Control_):
                result[k] = v.name_
            else:
                result[k] = v
    return json.dumps(result)
