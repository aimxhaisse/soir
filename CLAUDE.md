# Soir Development Guide

## Project Overview

Soir is a Live Coding Environment for Music Creation featuring:
- Real-time audio synthesis and processing
- C++ audio engine with Python bindings via pybind11
- Python frontend with CLI (typer), TUI (textual), and web interface (Flask)
- DSP effects, samplers, MIDI support, VST plugin hosting, and live coding capabilities

## Repository Structure

```
soir/
├── cpp/                    # C++ source code
│   ├── audio/              # Audio I/O, buffers, PCM streams, recording
│   ├── bindings/           # pybind11 Python bindings (rt, pcm, logger, soir, state)
│   ├── core/               # Engine, tracks, ADSR, MIDI, parameters, samples
│   ├── dsp/                # Filters, LFOs, reverb, delay, chorus, biquad
│   ├── fx/                 # Effects stack (chorus, echo, HPF, LPF, reverb, VST)
│   ├── inst/               # Instruments (sampler, external MIDI, VST)
│   ├── rt/                 # Runtime system
│   ├── utils/              # Config, logging, fast_random, tools
│   ├── vst/                # VST host, plugin wrapper, scanner, platform editors
│   └── tests/              # C++ unit tests (GoogleTest)
│       ├── audio/          # audio_buffer_test, audio_output_test
│       ├── core/           # core_test, engine_test
│       ├── dsp/            # delay_test, effects_test, filters_test, tools_test
│       ├── inst/           # sampler_test
│       └── utils/          # config_test, tools_test
├── py/                     # Python package
│   ├── soir/               # Main package (version 0.11.0)
│   │   ├── cast/           # Cast server for real-time JSON API
│   │   ├── cli/            # CLI commands
│   │   │   ├── samples.py  # Sample pack management
│   │   │   ├── session.py  # Session management
│   │   │   ├── utils.py    # CLI utilities
│   │   │   ├── www.py      # Web server command
│   │   │   └── tui/        # Terminal UI (textual)
│   │   │       ├── app.py          # Main TUI application (SoirTuiApp)
│   │   │       ├── commands.py     # Command interpreter
│   │   │       ├── engine_manager.py
│   │   │       └── widgets/        # Custom Textual widgets
│   │   ├── rt/             # Runtime Python API (user-facing)
│   │   │   ├── bpm.py, ctrls.py, errors.py, fx.py
│   │   │   ├── levels.py, midi.py, rnd.py, sampler.py
│   │   │   ├── system.py, tracks.py, vst.py
│   │   │   └── _ctrls.py, _helpers.py, _internals.py, _system.py
│   │   ├── www/            # Flask web application
│   │   ├── _data/          # Wheel-packaged resources (etc/, lib/samples symlinks)
│   │   ├── _launcher.py    # Console-script entry: re-exec under -Xgil=0, free-threaded check
│   │   ├── _resources.py   # Resources class for read-only wheel paths
│   │   ├── app.py          # Typer CLI entry point
│   │   ├── config.py       # Pydantic configuration models, $SOIR_HOME bootstrap
│   │   └── watcher.py      # File watching for live coding
│   └── tests/              # Python tests (pytest)
│       ├── integration/    # Integration tests (full session tests)
│       └── test_*.py       # Unit tests (config, watcher, www)
├── etc/                    # Default configuration files (also exposed via py/soir/_data/etc symlink)
│   ├── config.json         # Global engine configuration ($SOIR_HOME-aware paths)
│   └── live.default.py     # Default live coding template
├── build/                  # CMake build output (gitignored)
├── designs/                # Design documents (e.g. VST3.md)
├── lib/                    # Library resources
├── playground/             # Experimental/demo code (excluded from linting)
├── CMakeLists.txt          # C++ build configuration
├── justfile                # Build and development commands
├── pyproject.toml          # Python package configuration (version 1.0.0)
└── setup.py                # Custom build script (CMake integration)
```

## Python Environment

**CRITICAL: Always use `uv run` for all Python commands**

```bash
# Initial setup
just setup                  # Install Python 3.14.2t and sync dependencies

# Run Python code
uv run python script.py
uv run python -m soir.module

# Run CLI commands
uv run soir                 # Show help
uv run soir session mk demo # Create new session
uv run soir session run demo # Run session
uv run soir www             # Start web interface
```

After `pip install` / `uv tool install` of the wheel, the `soir` console
script re-execs under `-Xgil=0` automatically (see `py/soir/_launcher.py`).

## Build System

All commands use `just` (justfile):

```bash
just                          # List all available commands
just setup                    # Install Python 3.14.2t + sync dependencies
just build                    # Build C++ extension and install dev dependencies
just test                     # Run all tests (C++ unit + Python unit + integration)
just test-unit                # Run C++ tests (GoogleTest) + Python unit tests
just test-integration         # Run integration tests
just test-integration "BPM*"  # Run integration tests matching a pattern
just check                    # Format and lint (black, ruff, mypy, clang-format)
just clean                    # Remove build artifacts, .venv, __pycache__, *.so
just wheel                    # Build a cp314t wheel into dist/
```

**After any code change, always run:**
```bash
just build    # if C++ was changed
just check    # always
just test     # always
```

## Testing

### C++ Tests
- Framework: GoogleTest v1.17.0
- Tests in: `cpp/tests/`
- Run: `just test-unit` or `uv run python setup.py run_cpp_tests`
- Test executables: `utils_test`, `audio_test`, `core_test`, `engine_test`, `inst_test`, `dsp_test`

### Python Tests
- Framework: pytest
- Tests in: `py/tests/`
- Unit tests: `test_config.py`, `test_watcher.py`, `test_www.py`
- Integration tests: `py/tests/integration/`
- Run specific: `just test-integration "TestName*"`

## Code Style

### C++
- Standard: C++17
- Style: Google C++ Style Guide (see `.clang-format`: BasedOnStyle=Google, Standard=c++20)
- Naming:
  - `snake_case` for variables and functions
  - `CamelCase` for classes
  - Private members: `snake_case_` with trailing underscore (e.g. `attackMs_`)
  - `UPPERCASE` for enum values
  - Files: `.hh` headers, `.cc` implementation
- Error handling: `absl::Status`, `absl::StatusOr`; pattern: `if (!status.ok()) { LOG(ERROR) << "..."; return status; }`
- Logging: abseil logging — `LOG(INFO)`, `LOG(ERROR)`, `LOG(WARNING)`
- Namespaces: `namespace soir { ... }  // namespace soir` (nested: `namespace soir::vst`)
- Headers: `#pragma once`, no inline implementation (trivial getters excepted)
- Files in CMakeLists.txt: alphabetical order

### Python
- Standard: Python 3.14.2 (free-threaded build, `-Xgil=0`)
- Type hints: required, use native types — `str | Path`, `list[str]`, `dict[str, T]` (no `typing.Union/List/Dict`)
- Imports: always at the top of the file; never use `TYPE_CHECKING` guards — import directly and use `cast()` when needed for type narrowing
- Naming: `snake_case` functions/variables, `CamelCase` classes
- Docstrings: Google style (Args/Returns/Raises sections)
- Line length: 88 characters (black)
- Import order: stdlib → third-party → project
- Error handling: custom exceptions from `soir.rt.errors`
- mypy: strict mode enabled

## Key Technologies

### C++ Dependencies (via CMake FetchContent)
- pybind11 3.0.1 — Python bindings
- abseil-cpp 20250814.1 — Core utilities, status handling, logging
- nlohmann/json 3.12.0 — JSON parsing
- googletest 1.17.0 — Unit testing
- miniaudio 0.11.23 — Audio I/O
- AudioFile 1.1.4 — Audio file reading
- libremidi 5.3.1 — MIDI support
- VST3 SDK 3.8.0_build_66 — VST plugin hosting
- ogg 1.3.5 — Ogg format support
- opus 1.5.2 — Opus codec
- cpp-httplib 0.18.3 — HTTP client/server

### Python Dependencies
- typer — CLI framework
- textual — Terminal UI framework
- flask + gunicorn — Web framework and WSGI server
- pydantic — Configuration models and data validation
- pygments — Syntax highlighting
- watchdog — File system monitoring
- pdoc, markdown, markdown-it-py — Documentation and markdown parsing
- docstring-parser — Docstring parsing for runtime API
- soundfile — Audio file I/O (tests)
- pytest, pytest-flask, pytest-timeout — Testing

## Development Workflow

1. **Make changes** to C++ or Python code
2. **Build** if C++ changed: `just build`
3. **Format/lint**: `just check`
4. **Test**: `just test` or specific test commands
5. **Run**: `uv run soir session run <path>`

## Important Notes

- Python 3.14.2 required (free-threaded build with `-Xgil=0`)
- Virtual environment in `.venv/` managed by uv
- Never run plain `python` — always use `uv run python`
- C++ builds to `build/cmake/` directory
- Bindings output: `py/soir/_bindings.cpython-314t-{platform}.so`
- `$SOIR_HOME` (default: platformdirs user-data dir) holds user state — installed sample packs, per-user config. Tests set this env var to an isolated temp dir. The C++ engine expands `$SOIR_HOME` in path-typed config values via `Config::ExpandEnvironmentVariables`.
- Integration tests require audio configuration
- `playground/` is excluded from ruff linting
- Always run `just build` (if C++ changed), `just check`, and `just test` after proposing changes
