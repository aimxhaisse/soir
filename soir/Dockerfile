FROM ubuntu:22.04 AS base

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -yq &&       \
    apt-get install -yq         \
      build-essential           \
      musl-dev                  \
      libffi-dev                \
      python3-dev               \
      python3-pip               \
      python3-cryptography      \
      cron                      \
      xvfb                      \
      ffmpeg                    \
      sox                       \
      cmake                     \
      git                       \
      pkg-config                \
      clang-11                  \
      cmake                     \
      ninja-build               \
      libjack-jackd2-dev        \
      ladspa-sdk                \
      libcurl4-openssl-dev      \
      libfreetype6-dev          \
      libx11-dev                \
      libxcomposite-dev         \
      libasound2-dev            \
      libxcursor-dev            \
      libxext-dev               \
      libxinerama-dev           \
      libxrandr-dev             \
      libxrender-dev            \
      libwebkit2gtk-4.0-dev     \
      libglu1-mesa-dev          \
      mesa-common-dev           \
      libxcb-cursor0            \
      libyaml-cpp-dev           \
      libgoogle-glog-dev        \
      libxkbcommon-x11-dev	\
      libshout-dev              \
      libsfml-dev               \
      wget

RUN DEBIAN_FRONTEND=noninteractive                                            \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-11 100 &&     \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-11 100

WORKDIR /waves

# We try to be a bit smart here to avoid rebuilding everything on each change.

COPY Makefile Makefile

RUN make juce

COPY src src
COPY etc etc

COPY entrypoint.bash entrypoint.bash

ENV DISPLAY ":99"

CMD ["./entrypoint.bash"]
