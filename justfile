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
build:
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
test filter='*':
    poetry run make test TEST_FILTER={{filter}}

# Build and push documentation to soir.sbrk.org.
[group('dev')]
mkdocs:
    #!/usr/bin/env bash
    tmp=$(mktemp -d)
    mkdir -p "${tmp}/etc"
    just --justfile {{ justfile() }} prepare-config ${SOIR_DIR} ${tmp} "scripts.yaml.template"
    poetry -C ${SOIR_DIR} run -C ${SOIR_DIR} ${SOIR_BIN_DIR}/soir-engine --config "${tmp}/etc/config.yaml" --mode script --script scripts/mk-docs.py
    rm -rf ${tmp}

# Build the package for Soir.
[group('dev')]
package: build
    #!/usr/bin/env bash
    git fetch --tags
    TAG=$(git describe --tags `git rev-list --tags --max-count=1`)
    RELEASE="soir-${TAG}-{{ arch() }}-{{ os() }}"
    cp build/soir dist/bin/soir-engine
    tar -czf ${RELEASE}.tar.gz dist
