_default:
    @just --list --unsorted

# Builds neon
build:
    poetry run make

# Builds & run neon unit tests
test:
    poetry run make test
