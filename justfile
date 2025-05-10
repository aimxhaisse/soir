# --- This file is the main entrypoint for anything related to development. ---

set allow-duplicate-recipes := true
set allow-duplicate-variables := true

export SOIR_DIR := "./dist"

_default:
    @just --list --unsorted --justfile {{ justfile() }}

# Setup the virtualenv for Soir.
_venv:
    $SOIR_DIR/bin/soir --setup-only

# Build Soir.
[group('dev')]
dev-build:
    #!/usr/bin/env bash
    if ! [ -f build ]
    then
       mkdir -p build
       cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -B build -G Ninja
    fi
    cmake --build build --parallel 16
    cp build/soir dist/bin/soir-core

# Runs the Soir unit test suites.
[group('dev')]
dev-test filter='*': dev-build _venv
    #!/usr/bin/env bash
    source ${SOIR_DIR}/venv/bin/activate
    build/src/core/soir_core_test --gtest_filter='{{filter}}'
    build/src/utils/soir_utils_test --gtest_filter='{{filter}}'

# Build and push documentation to soir.dev.
[group('dev')]
dev-mk-docs:
    #!/usr/bin/env bash
    source ${SOIR_DIR}/venv/bin/activate
    cp install.sh www/site/

# Push the documentation to soir.sh.
[group('dev')]
dev-sync-docs:
    #!/usr/bin/env bash
    rsync -avz --delete dist/www/site/ soir.dev:srv/services/soir.dev/data

# Format the C++/Python code.
[group('dev')]
dev-fmt: _venv
    #!/usr/bin/env bash
    find src/ -name "*.cc" -o -name "*.h" | xargs clang-format -i --style=file
    uv run --python $SOIR_DIR/venv black $SOIR_DIR/py

# Build the package for Soir.
[group('dev')]
dev-mk-package: dev-build
    #!/usr/bin/env bash
    git fetch --tags
    TAG=$(git describe --tags `git rev-list --tags --max-count=1`)
    RELEASE="soir-${TAG}-{{ arch() }}-{{ os() }}"
    cp build/soir dist/bin/soir-core

    # Here we are explicit so that we can be sure we don't embed assets
    # or other files that are not needed.
    tar -czf ${RELEASE}.tar.gz \
        dist/bin \
        dist/etc \
        dist/py/__init__.py \
        dist/py/soir/*.py \
        dist/samples/registry.json \
        dist/pyproject.toml \
        dist/live.py.example
