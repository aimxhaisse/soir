#!/usr/bin/env bash

set -eo pipefail

BASE_DIR="${XDG_CONFIG_HOME:-$HOME}"
SOIR_DIR="${SOIR_DIR:-"$BASE_DIR/.soir"}"
SOIR_BIN_DIR="$SOIR_DIR/bin"

BIN_URL="https://raw.githubusercontent.com/aimxhaisse/soir/refs/heads/main/dist/bin/soir"
BIN_PATH="$SOIR_BIN_DIR/soir"

function install_dependencies {
    if [[ "$OSTYPE" =~ ^darwin ]]; then
        if ! command -v brew &> /dev/null; then
            echo "soir: homebrew is required to install dependencies."
            exit 1
        fi
        brew install --quiet just jq
    fi
}

function install_binaries {
    mkdir -p "$SOIR_BIN_DIR"
    curl -sSf -L "$BIN_URL" -o "$BIN_PATH"
    chmod +x "$BIN_PATH"
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

install_dependencies
install_binaries
install_path

echo "soir was installed successfully to ${SOIR_BIN_DIR}"
