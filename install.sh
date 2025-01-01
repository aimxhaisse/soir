#!/usr/bin/env bash
# Soir installer.
#
# Some bits are inspired from the uv and the cast installer
# scripts. Goal is to setup the dist directory in $SOIR_DIR (typically
# under the $HOME/.soir directory and add the soir bin directory to
# the PATH.
#
# It also looks for dependencies like `just` and `jq`, it doesn't try
# to install them as it may be complex to handle all cases, goal is to
# cover those in the documentation.

SOIR_DIR=${SOIR_DIR:-$HOME/.soir}

# Usage of the soir installer
function usage() {
    cat <<EOF
Usage: $0 [options]

Options:
  -h, --help          Show this help
EOF
}

# Check if a command exists
function check_cmd() {
    command -v "$1" > /dev/null 2>&1

    return $?
}

# Print an error message to stderr and exit if command doesn't exist
function need_cmd() {
    if ! check_cmd "$1"
    then
        err "need '$1' (command not found)"
    fi
}

# Print an error message to stderr and exit
function err() {
    local red
    local reset

    red=$(tput setaf 1 2>/dev/null || echo '')
    reset=$(tput sgr0 2>/dev/null || echo '')

    echo "${red}ERROR${reset}: $1" >&2

    exit 1
}

# Run a command or print an error message and exit
function ensure() {
    if ! "$@"
    then
        err "command failed: $*"
    fi
}

# Get the architecture of the system
function get_arch {
    need_cmd uname

    case $(uname -m) in
    arm64)
        echo "aarch64"
        ;;
    *)
        err "architecture not supported."
        exit 1
        ;;
    esac
}

# Get the operating system of the system
function get_os {
    need_cmd uname

    case $(uname -s) in
    Darwin)
        echo "macos"
        ;;
    *)
        err "operating system not supported."
        exit 1
        ;;
    esac
}

# Check dependencies
function check_deps {
    need_cmd python3.11
    need_cmd poetry
    need_cmd jq
    need_cmd curl
    need_cmd just
    need_cmd ffmpeg
}

# Download and install soir
function download_and_install {
    local os
    local arch
    local tag
    local release

    os=$(get_os)
    arch=$(get_arch)
    tag=$(curl -s "https://api.github.com/repos/aimxhaisse/soir/releases/latest" | jq -r .tag_name)
    release="soir-${tag}-${arch}-${os}"

    ensure mkdir -p "${SOIR_DIR}"
    ensure curl -sL "https://github.com/aimxhaisse/soir/releases/download/${tag}/${release}.tar.gz" -o "${SOIR_DIR}/soir.tar.gz"
    ensure tar -xf "${SOIR_DIR}/soir.tar.gz" -C "${SOIR_DIR}"
    ensure rm -f "${SOIR_DIR}/soir.tar.gz"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -s|--with-samples)
            WITH_SAMPLES=1
            ;;
        *)
            err "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

check_deps && download_and_install

echo "> █▀ █▀█ █ █▀█"
echo "> ▄█ █▄█ █ █▀▄ installed to ${SOIR_DIR}"
echo ">"
echo "> Add the following line to your shell configuration file:"
echo ""
echo "export SOIR_DIR=\"${SOIR_DIR}\""
echo "export PATH=\"${SOIR_DIR}/bin:\$PATH\""
echo ""
echo "> Then restart your shell and run 'soir'."
