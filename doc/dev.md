# Soir Developer Guide

## Development Environment

Only MacOS X is supported as a development environment:

    # install a few deps
    brew install clang-format
    brew install sfml
    brew install yaml-cpp
    brew install gflags
    brew install glog
    brew install rtmidi

    # (optional) install googletest
    brew install --HEAD https://gist.githubusercontent.com/Kronuz/96ac10fbd8472eb1e7566d740c4034f8/raw/gtest.rb

    # build soir
    make
