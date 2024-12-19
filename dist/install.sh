#!/usr/bin/env bash

set -eo pipefail

BASE_DIR="${XDG_CONFIG_HOME:-$HOME}"
SOIR_DIR="${SOIR_DIR:-"$BASE_DIR/.soir"}"
SOIR_BIN_DIR="$SOIR_DIR/bin"

TAG=$(curl -s "https://api.github.com/repos/aimxhaisse/soir/releases/latest" | jq -r .tag_name)
RELEASE="soir-${TAG}-{{ arch() }}-{{ os() }}"

function install_deps {
    if [[ "$OSTYPE" =~ ^darwin ]]; then
        if ! command -v brew &> /dev/null; then
            echo "soir: homebrew is required to install dependencies."
            exit 1
        fi
        brew install --quiet just jq python@3.11 poetry curl
    fi
}

function install_dist {
    echo "Setting up distribution for ${TAG}"
    cd "${SOIR_DIR}"
    curl -s "https://github.com/aimxhaisse/soir/archive/refs/tags/${TAG}.tar.gz" -o "${SOIR_DIR}/soir-${TAG}.tar.gz"
    tar -xzf "${SOIR_DIR}/soir-${TAG}.tar.gz" --strip-components=1 -C "${SOIR_DIR}"
}

function install_py {
    echo "Setting up Python dependencies for ${TAG}"
    poetry -C ${SOIR_DIR} env use -C ${SOIR_DIR} python3.11 -q
    poetry -C ${SOIR_DIR} install -C ${SOIR_DIR} -q
}

function install_path {
    case $SHELL in
    */zsh)
        PROFILE="${ZDOTDIR-"$HOME"}/.zshenv"
        PREF_SHELL=zsh
        ;;
    */bash)
        PROFILE=$HOME/.bashrc
        PREF_SHELL=bash
        ;;
    */fish)
        PROFILE=$HOME/.config/fish/config.fish
        PREF_SHELL=fish
        ;;
    */ash)
        PROFILE=$HOME/.profile
        PREF_SHELL=ash
        ;;
    *)
        echo "soir: could not detect shell, manually add ${SOIR_BIN_DIR} to your PATH."
        exit 1
    esac

    if [[ ":$PATH:" != *":${SOIR_BIN_DIR}:"* ]]; then
        if [[ "$PREF_SHELL" == "fish" ]]; then
            echo >> "$PROFILE" && echo "fish_add_path -a $SOIR_BIN_DIR" >> "$PROFILE"
        else
            echo >> "$PROFILE" && echo "export PATH=\"\$PATH:$SOIR_BIN_DIR\"" >> "$PROFILE"
        fi
    fi
}

echo "installing soir..."

install_deps
install_dist
install_py
install_path

echo "soir was installed successfully to ${SOIR_BIN_DIR}"
