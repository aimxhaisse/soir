# Agent Development Guide

## Project Overview

Soir is a Live Coding Environment for Music Creation with:

- Python frontend (Flask web app, CLI)
- C++ audio engine with pybind11 bindings
- Mixed C++/Python architecture

## Python Environment

**CRITICAL: Always use `uv run` to execute Python code in this project**

```bash
# Run Python scripts/modules
uv run python script.py
uv run python -m soir.module

# Run tests
uv run pytest

# Install dependencies
uv sync --all-extras
```

## Key Commands

- Build: `just build` (installs package in development mode)
- Test: `just test` or `uv run pytest`
- Format: `just check`
- Run CLI: `uv run soir` or `uv run python -m soir.cli`

## Project Structure

```
soir/                # Python package
├── cli.py           # Main CLI entry point
└── www/             # Flask web application
src/                 # C++ source files
tests/               # Python tests
```

## Important Notes

- Python 3.13+ required
- Virtual environment managed by uv (.venv/)
- Never run plain `python` - always use `uv run python`
- C++ components built via setuptools/pybind11
