# Soir Development Guide

## Project Overview

Soir is a Live Coding Environment for Music Creation featuring:
- Real-time audio synthesis and processing
- C++ audio engine (soir_core) with Python bindings via pybind11
- Python frontend with CLI (typer), TUI (textual), and web interface (Flask)
- DSP effects, samplers, MIDI support, and live coding capabilities

## Repository Structure

```
nocturne/
├── cpp/                    # C++ source code
│   ├── audio/              # Audio I/O, buffers, recording
│   ├── bindings/           # pybind11 Python bindings
│   ├── core/               # Engine, tracks, MIDI, samples
│   ├── dsp/                # Filters, LFOs, reverb, delay
│   ├── fx/                 # Effects stack (chorus, echo, etc.)
│   ├── inst/               # Instruments (sampler, MIDI)
│   ├── rt/                 # Runtime system
│   ├── utils/              # Config, logging, tools
│   └── tests/              # C++ unit tests (GoogleTest)
├── py/                     # Python package
│   ├── soir/               # Main package
│   │   ├── cli/            # CLI commands
│   │   │   ├── samples.py  # Sample pack management
│   │   │   ├── session.py  # Session management
│   │   │   ├── tui/        # Terminal UI (textual)
│   │   │   └── www.py      # Web server command
│   │   ├── rt/             # Runtime Python API
│   │   ├── www/            # Flask web application
│   │   ├── app.py          # Main entry point (typer)
│   │   ├── config.py       # Configuration management
│   │   └── watcher.py      # File watching for live coding
│   └── tests/              # Python tests (pytest)
│       ├── integration/    # Integration tests
│       └── test_*.py       # Unit tests
├── bin/                    # Executable wrapper scripts
├── build/                  # CMake build output (gitignored)
├── CMakeLists.txt          # C++ build configuration
├── justfile                # Build and development commands
├── pyproject.toml          # Python package configuration
└── setup.py                # Custom build script (CMake integration)
```

## Python Environment

**CRITICAL: Always use `uv run` for all Python commands**

```bash
# Initial setup
just setup                  # Install Python 3.14t and sync dependencies

# Run Python code
uv run python script.py
uv run python -m soir.module

# Run CLI commands
uv run soir                 # Show help
uv run soir session mk demo # Create new session
uv run soir session run demo # Run session
uv run soir www             # Start web interface

# Or use the wrapper script
./bin/soir session run demo
```

## Build System

All commands use `just` (justfile):

```bash
just                        # List all available commands
just setup                  # Install Python 3.14t + dependencies
just build                  # Build C++ extension with tests
just test                   # Run all tests (C++ + Python)
just test-unit              # Run unit tests only
just test-integration       # Run integration tests only
just test-integration "BPM*" # Run specific integration test pattern
just check                  # Format and lint (black, ruff, mypy, clang-format, clang-tidy)
just clean                  # Remove build artifacts
just package                # Create distributable package
```

## Testing

### C++ Tests
- Framework: GoogleTest
- Tests in: `cpp/tests/`
- Run: `just test-unit` or `uv run python setup.py run_cpp_tests`
- Categories: audio, core, dsp, inst, utils

### Python Tests
- Framework: pytest
- Tests in: `py/tests/`
- Unit tests: `test_config.py`, `test_watcher.py`, `test_www.py`
- Integration tests: `py/tests/integration/`
- Run specific: `just test-integration "TestName*"`

## Code Style

### C++
- Standard: C++17
- Style: Google C++ Style Guide (see `.clang-format`)
- Naming:
  - `snake_case` for variables and functions
  - `CamelCase` for classes
  - Files: `.hh` headers, `.cc` implementation
- Error handling: `absl::Status`, `absl::StatusOr`
- Namespaces: `soir::{component}` (separate lines)
- No inline implementation in headers
- Files in CMakeLists.txt: alphabetical order

### Python
- Standard: Python 3.14.0 (requires free-threaded build)
- Type hints: Required, use native types (no `typing` imports)
- Naming: `snake_case` functions/variables, `CamelCase` classes
- Docstrings: Google style (Args/Returns/Raises)
- Line length: 88 characters (black)
- Import order: stdlib → third-party → project
- Error handling: Custom exceptions from `soir.rt.errors`

## Key Technologies

### C++ Dependencies (via CMake FetchContent)
- pybind11 3.0.1: Python bindings
- abseil-cpp: Core utilities, status handling
- nlohmann/json: JSON parsing
- googletest: Unit testing
- miniaudio: Audio I/O
- AudioFile: Audio file reading
- libremidi: MIDI support
- rapidjson: Fast JSON parsing

### Python Dependencies
- typer: CLI framework
- flask: Web framework
- pygments: Syntax highlighting
- pydantic: Data validation
- watchdog: File system monitoring
- pytest: Testing framework

## Development Workflow

1. **Make changes** to C++ or Python code
2. **Build** if C++ changed: `just build`
3. **Format/lint**: `just check`
4. **Test**: `just test` or specific test commands
5. **Run**: `uv run soir session run <path>` or `./bin/soir session run <path>`

## Important Notes

- Python 3.14.0 required (free-threaded build with `-Xgil=0`)
- Virtual environment in `.venv/` managed by uv
- Never run plain `python` - always use `uv run python`
- C++ builds to `build/cmake/` directory
- Bindings output: `py/soir/_bindings.cpython-314t-darwin.so`
- The `bin/soir` wrapper disables GIL for thread safety
- Integration tests require audio configuration
