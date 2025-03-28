# --- Builder/development part
#
# Goal is to get rid of the makefile at some point and only rely on
# this justfile.

set allow-duplicate-recipes := true
set allow-duplicate-variables := true

export SOIR_DIR := "./dist"

import 'dist/bin/soir'
import 'pkg/justfile'

_default:
    @just --list --unsorted --justfile {{ justfile() }}

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
    cp build/soir dist/bin/soir-engine

# Runs the Soir unit test suites.
[group('dev')]
dev-test filter='*': dev-build
    #!/usr/bin/env bash
    just --justfile {{ justfile() }} prepare-config dist "scripts.yaml.template"
    poetry -P ${SOIR_DIR} run build/src/core/soir_core_test --gtest_filter='{{filter}}'
    poetry -P ${SOIR_DIR} run build/src/utils/soir_utils_test --gtest_filter='{{filter}}'

# Build and push documentation to soir.dev.
[group('dev')]
dev-mk-docs:
    #!/usr/bin/env bash
    just --justfile {{ justfile() }} prepare-config dist "scripts.yaml.template"
    poetry -P ${SOIR_DIR} run ${SOIR_BIN_DIR}/soir-engine --config dist/etc/config.yaml --mode script --script scripts/mk-docs.py
    cp install.sh www/site/

# Run the Soir engine in audio mode.
[group('dev')]
dev-run:
    #!/usr/bin/env bash
    just --justfile {{ justfile() }} prepare-config dist "config.yaml.template"
    poetry -P ${SOIR_DIR} run ${SOIR_BIN_DIR}/soir-engine --config dist/etc/config.yaml --mode standalone

# Push the documentation to soir.sh.
[group('dev')]
dev-sync-docs: dev-mk-docs
    #!/usr/bin/env bash
    rsync -avz --delete www/site/ soir.dev:srv/services/soir.dev/data

# Format the C++/Python code.
[group('dev')]
dev-fmt:
    #!/usr/bin/env bash
    find src/ -name "*.cc" -o -name "*.h" | xargs clang-format -i --style=file
    poetry -P ${SOIR_DIR} run black dist/py/soir/*.py

# Build the package for Soir.
[group('dev')]
dev-mk-package: dev-build
    #!/usr/bin/env bash
    git fetch --tags
    TAG=$(git describe --tags `git rev-list --tags --max-count=1`)
    RELEASE="soir-${TAG}-{{ arch() }}-{{ os() }}"
    cp build/soir dist/bin/soir-engine

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
