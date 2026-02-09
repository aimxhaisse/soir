"""Documentation extraction and parsing module.

This module handles extracting documentation from Python modules,
parsing docstrings, and building structured documentation data.
"""

import importlib
import inspect

from docstring_parser import Docstring, parse as parse_docstring
from pydantic import BaseModel, ConfigDict, Field


class MethodDoc(BaseModel):
    model_config = ConfigDict(arbitrary_types_allowed=True)

    name: str
    signature: str | None
    parsed: Docstring


class MemberDoc(BaseModel):
    model_config = ConfigDict(arbitrary_types_allowed=True)

    name: str
    type: str
    signature: str | None
    parsed: Docstring
    methods: list[MethodDoc] = Field(default_factory=list)


class ModuleDoc(BaseModel):
    model_config = ConfigDict(arbitrary_types_allowed=True)

    name: str
    full_path: str
    parsed: Docstring
    members: list[MemberDoc]


def is_public(doc: str) -> bool:
    return "@public" in doc


def extract_module_docs(module_path: str) -> ModuleDoc:
    module = importlib.import_module(module_path)

    module_doc_raw = inspect.getdoc(module)
    module_parsed = parse_docstring(str(module_doc_raw))

    members = []
    for name, obj in inspect.getmembers(module):
        is_private = name.startswith("_")
        if is_private:
            continue

        is_imported = hasattr(obj, "__module__") and obj.__module__ != module_path
        if is_imported:
            continue

        doc = inspect.getdoc(obj)
        if doc is None or not is_public(doc):
            continue

        parsed = parse_docstring(doc.replace("@public", ""))

        signature = None
        if callable(obj):
            try:
                signature = str(inspect.signature(obj))
            except ValueError, TypeError:
                # Some builtin types (like exception classes) don't have
                # accessible signatures in Python 3.14+
                signature = None

        if inspect.isfunction(obj):
            if name in ["live", "loop"]:
                member_type = "decorator"
            else:
                member_type = "function"
        elif inspect.isclass(obj):
            member_type = "class"
        else:
            continue

        methods = []
        if inspect.isclass(obj):
            for method_name, method_obj in inspect.getmembers(obj):
                is_private = method_name.startswith("_")
                if is_private:
                    continue

                method_doc = inspect.getdoc(method_obj)
                if method_doc is None or not is_public(method_doc):
                    continue
                method_parsed = parse_docstring(method_doc.replace("@public", ""))

                method_signature = None
                if callable(method_obj):
                    try:
                        sig = inspect.signature(method_obj)
                        method_signature = str(sig)
                    except ValueError, TypeError:
                        # Some builtin types don't have accessible signatures
                        method_signature = None

                methods.append(
                    MethodDoc(
                        name=method_name,
                        signature=method_signature,
                        parsed=method_parsed,
                    )
                )

        member_data = MemberDoc(
            name=name,
            type=member_type,
            signature=signature,
            parsed=parsed,
            methods=methods,
        )

        members.append(member_data)

    return ModuleDoc(
        name=module_path.split(".")[0],
        full_path=module_path,
        parsed=module_parsed,
        members=members,
    )


class DocsCache:
    """Documentation cache for storing pre-generated module doc.

    This class provides a simple cache for module documentation to
    avoid re-extracting documentation on every request.
    """

    def __init__(self) -> None:
        self._cache: dict[str, ModuleDoc] = {}

    def get(self, module_name: str) -> ModuleDoc | None:
        return self._cache.get(module_name)

    def set(self, module_name: str, docs: ModuleDoc) -> None:
        self._cache[module_name] = docs

    def has(self, module_name: str) -> bool:
        return module_name in self._cache

    def clear(self) -> None:
        self._cache.clear()

    def __len__(self) -> int:
        return len(self._cache)
