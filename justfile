_default:
    @just --choose

# --- Builder/development part
#
# Goal is to get rid of the makefile at some point and only rely on
# this justfile.

build:
    poetry run make build

test filter='*':
    poetry run make test TEST_FILTER={{filter}}

doc:
    poetry run make push

# --- User/session part
#
# To be migrated to a dedicated justfile that will be the entrypoint
# to performances/initialization/setup.
