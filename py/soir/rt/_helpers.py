import json

from soir.rt._ctrls import (
    Control_,
)


class ControlEncoder(json.JSONEncoder):
    """Custom JSON encoder that serializes Control objects to their names.

    This allows Control references to be used anywhere in a JSON structure,
    including nested dicts. The C++ side expects control names as strings
    and will resolve them to actual control values at runtime.
    """

    def default(self, obj: object) -> object:
        if isinstance(obj, Control_):
            return obj.name_
        return super().default(obj)


def serialize_parameters(params: dict[str, object] | None) -> str:
    """Helper to serialize parameters to JSON.

    This is meant to pass parameters to arbitrary DSP code via JSON
    encoding. Control objects are automatically converted to their
    string names, which the C++ side resolves at runtime.
    """
    if params is None:
        return "{}"
    return json.dumps(params, cls=ControlEncoder)
