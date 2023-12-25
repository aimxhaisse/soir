#!/usr/bin/env bash
#
# Docker entrypoint for Waves.

make deps
make

# This is required so Dexed (vst) can work.
function setup_x_for_vst {
    rm -rf /tmp/.X*
    Xvfb :99 &
    sleep 3
}

# On GNU/Linux the properties path is under ~/.config.
function setup_blz_license {
    mkdir -p ~/BLZ\ Audio/
    cp ../data/waves/daw_data/Licenses/*.settings ~/BLZ\ Audio/
}

setup_blz_license
setup_x_for_vst

bin/waves --config etc/config-linux.yaml
