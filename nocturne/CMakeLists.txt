cmake_minimum_required(VERSION 3.15)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(soir_core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Release)

include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v3.0.1
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_Declare(
    abseil-cpp
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20250814.1
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23
)

FetchContent_Declare(
    audiofile
    GIT_REPOSITORY https://github.com/adamstark/AudioFile.git
    GIT_TAG 1.1.4
)

FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/jcelerier/libremidi.git
    GIT_TAG v5.3.1
)


set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)    # gtest
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # audiofile
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)    # audiofile

FetchContent_MakeAvailable(pybind11 nlohmann_json abseil-cpp googletest miniaudio audiofile libremidi)

# make sure pybind targets our environment

set(Python_ROOT_DIR ".venv")
find_package(Python COMPONENTS Interpreter Development)

# utils

add_library(soir_utils
    cpp/src/utils/config.cc
    cpp/src/utils/config.hh
    cpp/src/utils/fast_random.cc
    cpp/src/utils/logger.cc
    cpp/src/utils/logger.hh
    cpp/src/utils/tools.cc
)

target_include_directories(soir_utils PUBLIC cpp/src)

target_link_libraries(soir_utils
    nlohmann_json::nlohmann_json
    absl::base
    absl::strings
    absl::status
    absl::statusor
    absl::log
)

# audio

add_library(soir_audio
    cpp/src/audio/audio_buffer.cc
    cpp/src/audio/audio_output.cc
)

target_include_directories(soir_audio PUBLIC cpp/src ${miniaudio_SOURCE_DIR})

target_link_libraries(soir_audio
    soir_dsp
    absl::log
    absl::status
    absl::statusor
)

# core

add_library(soir_core_utils
    cpp/src/core/sample.cc
)

target_include_directories(soir_core_utils PUBLIC cpp/src)

target_link_libraries(soir_core_utils
    soir_dsp
    absl::status
    absl::statusor
)

# dsp

add_library(soir_dsp
    cpp/src/dsp/band_pass_filter.cc
    cpp/src/dsp/biquad_filter.cc
    cpp/src/dsp/chorus.cc
    cpp/src/dsp/comb_filter.cc
    cpp/src/dsp/delay.cc
    cpp/src/dsp/delayed_apf.cc
    cpp/src/dsp/high_pass_filter.cc
    cpp/src/dsp/high_shelving_filter.cc
    cpp/src/dsp/lfo.cc
    cpp/src/dsp/low_pass_filter.cc
    cpp/src/dsp/low_shelving_filter.cc
    cpp/src/dsp/lpf.cc
    cpp/src/dsp/modulated_delay.cc
    cpp/src/dsp/reverb.cc
    cpp/src/dsp/tools.cc
    cpp/src/dsp/two_band_shelving_filter.cc
)

target_include_directories(soir_dsp PUBLIC cpp/src)

target_link_libraries(soir_dsp
    soir_utils
    absl::log
    absl::status
    absl::statusor
)

# core

pybind11_add_module(_core
    cpp/bindings/bindings.cc
    cpp/bindings/logger.cc
    cpp/bindings/soir.cc
    cpp/src/core/soir.cc
)

target_include_directories(_core PRIVATE cpp/src cpp)

target_link_libraries(_core PRIVATE
    soir_utils
    absl::status
    absl::statusor
    absl::log
    absl::log_initialize
)

target_compile_options(_core PRIVATE
    -Os
    -fvisibility=hidden
)

set_target_properties(_core PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
)

# Unit tests
    
enable_testing()

add_executable(utils_test
    cpp/tests/utils/config_test.cc
    cpp/tests/utils/tools_test.cc
)

target_link_libraries(utils_test
    soir_utils
    gtest
    gtest_main
)

add_test(NAME UtilsTest COMMAND utils_test)

add_executable(audio_test
    cpp/tests/audio/audio_buffer_test.cc
    cpp/tests/audio/audio_output_test.cc
)

target_link_libraries(audio_test
    soir_audio
    gtest
    gtest_main
)

add_test(NAME AudioTest COMMAND audio_test)

add_executable(core_test
    cpp/tests/core/core_test.cc
)

target_link_libraries(core_test
    soir_core_utils
    gtest
    gtest_main
)

add_test(NAME CoreTest COMMAND core_test)

add_executable(dsp_test
    cpp/tests/dsp/delay_test.cc
    cpp/tests/dsp/effects_test.cc
    cpp/tests/dsp/filters_test.cc
    cpp/tests/dsp/tools_test.cc
)

target_link_libraries(dsp_test
    soir_dsp
    gtest
    gtest_main
)

add_test(NAME DspTest COMMAND dsp_test)
