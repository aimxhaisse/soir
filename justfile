# --- Builder/development part
#
# Goal is to get rid of the makefile at some point and only rely on
# this justfile.

set allow-duplicate-recipes := true

import 'dist/bin/soir'

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
    poetry -C ${SOIR_DIR} run -C ${SOIR_DIR} build/src/core/soir_core_test TEST_FILTER={{filter}}
    poetry -C ${SOIR_DIR} run -C ${SOIR_DIR} build/src/utils/soir_utils_test TEST_FILTER={{filter}}

# Build and push documentation to soir.dev.
[group('dev')]
dev-mk-docs:
    #!/usr/bin/env bash
    just --justfile {{ justfile() }} prepare-config dist "scripts.yaml.template"
    poetry -C ${SOIR_DIR} run -C ${SOIR_DIR} ${SOIR_BIN_DIR}/soir-engine --config dist/etc/config.yaml --mode script --script scripts/mk-docs.py
    cp install.sh www/site/

# Run the Soir engine in audio mode.
[group('dev')]
dev-run:
    #!/usr/bin/env bash
    just --justfile {{ justfile() }} prepare-config dist "config.yaml.template"
    poetry -C ${SOIR_DIR} run -C ${SOIR_DIR} ${SOIR_BIN_DIR}/soir-engine --config dist/etc/config.yaml --mode standalone

# Push the documentation to soir.sh.
[group('dev')]
dev-sync-docs: dev-mk-docs
    #!/usr/bin/env bash
    rsync -avz --delete www/site/ soir.dev:services/soir.dev/data

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
