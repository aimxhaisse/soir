_default:
    @just --list

# Create virtual environment and install Python
setup:
    uv python install 3.14.2t
    uv venv --python 3.14.2t --relocatable

# Clean build files
clean:
    #!/usr/bin/env bash

    rm -rf build/ .dist/ *.egg-info/ .venv/
    find . -name "*.pyc" -delete
    find . -name "__pycache__" -delete
    find py -name "*.so" -delete

# Format code & check code
check:
    #!/usr/bin/env bash

    uv run black .
    uv run ruff check . --preview --exclude etc --exclude playground
    uv run mypy py

    clang-format -i $(find cpp -name '*.cc') $(find cpp -name '*.hh')

# Build C++ extension and install dependencies
build:
    uv pip install .[dev]
    uv run python setup.py build_ext --inplace --with-tests

# Run unit tests only
test-unit:
    uv run python setup.py run_cpp_tests
    uv run pytest py/tests/test_config.py py/tests/test_watcher.py py/tests/test_www.py -v

# Run integration tests only
test-integration pattern="":
    uv run pytest -sv --timeout 360 py/tests/integration -v -x {{ if pattern != "" { "-k '" + pattern + "'" } else { "" } }}

# Run all tests
test:
    just test-unit
    just test-integration

# Build a wheel for the current platform + interpreter
wheel:
    rm -rf dist/
    uv build --wheel
    @ls -lh dist/
