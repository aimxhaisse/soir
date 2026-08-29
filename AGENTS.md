# Soir Development Guide

## Project Overview

Soir is a Live Coding Environment for Music Creation featuring:
- Real-time audio synthesis and processing
- C++ audio engine with Python bindings via pybind11
- Python frontend with CLI (typer), TUI (textual), and web interface (Flask)
- DSP effects, samplers, MIDI support, VST plugin hosting, and live coding capabilities
- Audio streaming (Ogg/Opus over HTTP, raw PCM over WebSocket) and WAV recording
- Sample pack system with a bundled set of self-authored CC0 `std-*` packs
- Documentation website (Flask + gunicorn) with client-side search, deployable via Docker

## Repository Structure

```
soir/
├── cpp/                    # C++ source code
│   ├── audio/              # Audio I/O, buffers, recording, PCM streams,
│   │                       #   HTTP/Ogg-Opus streaming (audio_http_server,
│   │                       #   audio_stream, ogg_opus_encoder, pcm_stream)
│   ├── bindings/           # pybind11 Python bindings (rt, pcm, logger, soir, state)
│   ├── core/               # Engine, tracks, ADSR, controls, level meter, MIDI
│   │                       #   (stack/sysex), parameters, sample manager/packs
│   ├── dsp/                # Filters, LFOs, reverb, delay, chorus, biquad
│   ├── fx/                 # Effects stack (chorus, echo, HPF, LPF, reverb, VST)
│   ├── inst/               # Instruments (sampler, external MIDI, VST) plus
│   │                       #   platform MIDI backends (midi_backend_{linux,macos,win})
│   ├── rt/                 # Runtime system
│   ├── utils/              # Config, logging, fast_random, tools
│   ├── vst/                # VST host, plugin wrapper, scanner, platform editors
│   └── tests/              # C++ unit tests (GoogleTest)
│       ├── audio/          # audio_buffer_test, audio_output_test
│       ├── core/           # core_test, engine_test
│       ├── dsp/            # delay_test, effects_test, filters_test, tools_test
│       ├── inst/           # midi_backend_test, sampler_test
│       └── utils/          # config_test, tools_test
├── py/                     # Python package
│   ├── soir/               # Main package (version 1.0.0)
│   │   ├── cast/           # CastServer: embedded per-session web server
│   │   │                   #   (WebGL visualizer, SSE engine state, WebSocket PCM)
│   │   ├── cli/            # CLI commands
│   │   │   ├── samples.py  # Sample pack management (ls, mk, install, rm, info)
│   │   │   ├── session.py  # Session management (run, mk); starts CastServer
│   │   │   ├── utils.py    # CLI utilities
│   │   │   ├── vst.py      # VST plugin management (ls)
│   │   │   ├── www.py      # Documentation website command (run)
│   │   │   └── tui/        # Terminal UI (textual)
│   │   │       ├── app.py            # Main TUI application (SoirTuiApp)
│   │   │       ├── app.tcss          # Textual stylesheet
│   │   │       ├── commands.py       # Command interpreter
│   │   │       ├── engine_manager.py
│   │   │       ├── log_tailer.py     # Engine log tailing for the log viewer
│   │   │       └── widgets/          # Custom Textual widgets (header, command
│   │   │                             #   shell, log viewer, info panel, audio device)
│   │   ├── rt/             # Runtime Python API (user-facing)
│   │   │   ├── bpm.py, ctrls.py, errors.py, fx.py
│   │   │   ├── levels.py, midi.py, rnd.py, sampler.py
│   │   │   ├── system.py (record, audio/MIDI device info), tracks.py, vst.py
│   │   │   └── _ctrls.py, _helpers.py, _internals.py, _system.py
│   │   ├── www/            # Documentation website (Flask)
│   │   │   ├── app.py      # Flask app + start_server (gunicorn in production)
│   │   │   ├── docs.py     # Docstring extraction (DocsCache)
│   │   │   ├── renderer.py # Markdown rendering
│   │   │   ├── content/    # Static pages (home, quickstart, examples, about)
│   │   │   ├── static/     # CSS + search.js (client-side search index)
│   │   │   └── templates/  # Jinja2 templates (reference, TOC, search)
│   │   ├── _data/          # Wheel-packaged resources (etc/, lib/samples)
│   │   ├── _launcher.py    # Console-script entry: re-exec under -Xgil=0, free-threaded check
│   │   ├── _resources.py   # Resources class for read-only wheel paths
│   │   ├── app.py          # Typer CLI entry point (samples, session, vst, www)
│   │   ├── config.py       # Pydantic configuration models (dsp, live, cast, vst),
│   │   │                   #   $SOIR_HOME bootstrap
│   │   └── watcher.py      # File watching for live coding
│   └── tests/              # Python tests (pytest)
│       ├── integration/    # Integration tests — SoirTestEngine (log-file
│       │                   #   notification waiting) in soir_test_base.py, unittest
│       │                   #   base in base.py; covers audio recording, bpm, cast,
│       │                   #   controls, levels, live coding, loops, samples,
│       │                   #   streaming, system, tracks, VST
│       └── test_*.py       # Unit tests (config, vst, watcher, www, session)
├── docker/                 # Docker deployment for the www site (multi-stage build)
├── etc/                    # Default configuration files (also exposed via py/soir/_data/etc symlink)
│   ├── config.json         # Global engine configuration ($SOIR_HOME-aware paths,
│   │                       #   dsp streaming keys, vst.scan_at_startup)
│   └── live.default.py     # Default live coding template
├── lib/samples/            # Standard sample packs (std-bass, std-drums, std-fx,
│                           #   std-leads, std-loops, std-pads) + registry.json;
│                           #   shipped in the wheel, provenance in CREDITS.md
├── build/                  # CMake build output (gitignored)
├── designs/                # Design documents (DISTRIBUTION.md, VST3.md)
├── playground/             # Experimental/demo code (excluded from linting)
├── CMakeLists.txt          # C++ build configuration
├── justfile                # Build and development commands
├── pyproject.toml          # Python package configuration (version 1.0.0)
├── setup.py                # Custom build script (CMake integration)
└── CREDITS.md              # Sample pack provenance and licenses
```

## Python Environment

**CRITICAL: Always use `uv run` for all Python commands**

```bash
# Initial setup
just setup                  # Install Python 3.14.2+freethreaded, create relocatable venv
just build                  # Installs .[dev] deps into the venv + builds C++ ext

# Run Python code
uv run python script.py
uv run python -m soir.module

# Run CLI commands
uv run soir                 # Show help
uv run soir session mk demo # Create new session
uv run soir session run demo # Run session (TUI + optional CastServer web UI)
uv run soir www run         # Start documentation website
uv run soir vst ls          # List detected VST3 plugins
uv run soir samples install std-drums  # Install a sample pack
```

After `pip install` / `uv tool install` of the wheel, the `soir` console
script re-execs under `-Xgil=0` automatically (see `py/soir/_launcher.py`).

## Build System

All commands use `just` (justfile):

```bash
just                          # List all available commands
just setup                    # Install Python 3.14.2+freethreaded + create relocatable venv
just build                    # `uv pip install .[dev]` + build C++ ext in-place (with tests)
just test                     # Run all tests (C++ unit + Python unit + integration)
just test-unit                # Run C++ tests (GoogleTest) + Python unit tests
just test-integration         # Run integration tests (pytest -sv --timeout 360 -x)
just test-integration "BPM*"  # Run integration tests matching a pattern
just check                    # Format and lint (black, ruff --preview, mypy, clang-format)
just clean                    # Remove build artifacts, .venv, __pycache__, *.so
just wheel                    # Build a cp314t wheel into dist/ (uv build --wheel)
```

Note: `just setup` only provisions the venv; dependencies are installed by
`just build` (`uv pip install .[dev]`).

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
- Unit tests (run by `just test-unit`): `test_config.py`, `test_vst.py`, `test_watcher.py`, `test_www.py`
- Integration tests: `py/tests/integration/`
  - `SoirTestEngine` in `soir_test_base.py` wraps the engine and waits for
    log-file notifications; `base.py` provides the `unittest` base classes
  - VST integration tests scan only the VST3 SDK sample plug-ins built under
    `build/cmake/` (never system-wide plug-ins, to keep tests deterministic)
  - VST editor tests require an X11 display; they skip themselves when
    `DISPLAY` is not set (headless/CI environments)
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
- **Platform-specific code must live in suffixed files, never behind `#ifdef` blocks in shared `.cc` files.** Use `_linux.cc`, `_macos.mm`, `_win.cc` (and matching `.hh` headers when needed). Add the platform files to the appropriate `target_sources` branch in `CMakeLists.txt`. Keep the base class / cross-platform interface in the unsuffixed file and provide a factory function (e.g. `CreateHostContext()`) that the shared code calls.

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
- cpp-httplib 0.18.3 — HTTP client/server (audio streaming)
- readerwriterqueue 1.0.7 — Lock-free queue (PCM streaming)

### Python Dependencies
- typer — CLI framework
- textual — Terminal UI framework
- flask + flask-sock + gunicorn — Web framework, WebSocket PCM streaming, WSGI server
- pydantic — Configuration models and data validation
- platformdirs — User data dir resolution ($SOIR_HOME)
- pygments — Syntax highlighting
- watchdog — File system monitoring
- pdoc, markdown, markdown-it-py — Documentation and markdown parsing
- docstring-parser — Docstring parsing for runtime API
- soundfile — Audio file I/O (tests)
- dev: pytest, pytest-flask, pytest-xdist, pytest-timeout, black, ruff, mypy, build

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
- Config sections: `dsp` (incl. `enable_streaming`, `streaming_host`, `streaming_port`, `streaming_bitrate`), `live`, `cast` (`enabled`, `port` — CastServer for `soir session run`), `vst` (`scan_at_startup`)
- The `std-*` sample packs ship in the wheel (`package-data`); user-installed packs go to `$SOIR_HOME/lib/samples`. `soir samples install` can fetch from the registry or unpack a local `.tar.gz` (ffmpeg is used to normalise audio formats)
- Documentation site can be deployed with Docker: `docker compose -f docker/docker-compose.yml up -d --build` serves it on host port 10011 with `$SOIR_HOME` on a named volume (see `docker/README.md`)
- Integration tests require audio configuration
- `playground/` is excluded from ruff linting
- Always run `just build` (if C++ changed), `just check`, and `just test` after proposing changes
