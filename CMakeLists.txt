cmake_minimum_required(VERSION 3.15)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(soir_core LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

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

FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.0_build_66
)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)    # gtest
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # audiofile
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)    # audiofile
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# VST3 SDK options
set(SMTG_ADD_VSTGUI OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_PLUGINS_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_HOSTING_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(pybind11 nlohmann_json abseil-cpp googletest miniaudio audiofile libremidi rapidjson vst3sdk)

# make sure pybind targets our environment

set(Python_ROOT_DIR ".venv")
find_package(Python COMPONENTS Interpreter Development)

# utils

add_library(soir_utils
    cpp/utils/config.cc
    cpp/utils/config.hh
    cpp/utils/fast_random.cc
    cpp/utils/logger.cc
    cpp/utils/logger.hh
    cpp/utils/tools.cc
)

target_include_directories(soir_utils PUBLIC cpp)

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
    cpp/audio/audio_buffer.cc
    cpp/audio/audio_output.cc
    cpp/audio/audio_recorder.cc
)

target_include_directories(soir_audio PUBLIC cpp ${miniaudio_SOURCE_DIR} ${audiofile_SOURCE_DIR})

target_link_libraries(soir_audio
    soir_dsp
    absl::log
    absl::status
    absl::statusor
)

# core

add_library(soir_core_utils
    cpp/core/adsr.cc
    cpp/core/controls.cc
    cpp/core/level_meter.cc
    cpp/core/midi_stack.cc
    cpp/core/midi_sysex.cc
    cpp/core/parameter.cc
    cpp/core/sample.cc
    cpp/core/sample_manager.cc
    cpp/core/sample_pack.cc
)

target_include_directories(soir_core_utils PUBLIC cpp ${audiofile_SOURCE_DIR} ${libremidi_SOURCE_DIR}/include)
target_include_directories(soir_core_utils SYSTEM PUBLIC ${rapidjson_SOURCE_DIR}/include)

target_link_libraries(soir_core_utils
    soir_audio
    soir_dsp
    soir_utils
    libremidi
    pybind11::pybind11
    absl::log
    absl::status
    absl::statusor
    absl::synchronization
    absl::time
)

# fx

add_library(soir_fx
    cpp/fx/fx_chorus.cc
    cpp/fx/fx_echo.cc
    cpp/fx/fx_hpf.cc
    cpp/fx/fx_lpf.cc
    cpp/fx/fx_reverb.cc
    cpp/fx/fx_stack.cc
    cpp/fx/fx_vst.cc
)

target_include_directories(soir_fx PUBLIC cpp ${vst3sdk_SOURCE_DIR})

target_link_libraries(soir_fx
    soir_audio
    soir_core_utils
    soir_dsp
    soir_vst
    absl::log
    absl::status
    absl::statusor
    absl::synchronization
)

# inst

add_library(soir_inst
    cpp/inst/external.cc
    cpp/inst/inst_vst.cc
    cpp/inst/sampler.cc
)

target_include_directories(soir_inst PUBLIC cpp ${audiofile_SOURCE_DIR} ${libremidi_SOURCE_DIR}/include ${miniaudio_SOURCE_DIR})
target_include_directories(soir_inst SYSTEM PUBLIC ${rapidjson_SOURCE_DIR}/include)

target_link_libraries(soir_inst
    soir_audio
    soir_core_utils
    soir_utils
    soir_vst
    libremidi
    absl::log
    absl::status
    absl::statusor
)

# vst

add_library(soir_vst
    cpp/vst/vst_host.cc
    cpp/vst/vst_plugin.cc
    cpp/vst/vst_scanner.cc
)

if(APPLE)
    target_sources(soir_vst PRIVATE
        cpp/vst/vst_editor_macos.mm
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_mac.mm
    )
    set_source_files_properties(
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_mac.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
elseif(UNIX)
    target_sources(soir_vst PRIVATE
        cpp/vst/vst_editor_linux.cc
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_linux.cpp
    )
endif()

target_include_directories(soir_vst PUBLIC cpp ${vst3sdk_SOURCE_DIR} ${libremidi_SOURCE_DIR}/include)

target_link_libraries(soir_vst
    soir_audio
    libremidi
    sdk
    sdk_hosting
    absl::log
    absl::status
    absl::statusor
    absl::synchronization
)

if(APPLE)
    target_link_libraries(soir_vst
        "-framework Cocoa"
        "-framework CoreFoundation"
    )
elseif(UNIX)
    target_link_libraries(soir_vst
        dl
    )
endif()

# engine

add_library(soir_engine
    cpp/core/engine.cc
    cpp/core/track.cc
)

target_include_directories(soir_engine PUBLIC cpp ${libremidi_SOURCE_DIR}/include ${vst3sdk_SOURCE_DIR})

target_link_libraries(soir_engine
    soir_audio
    soir_core_utils
    soir_fx
    soir_inst
    soir_vst
    libremidi
    absl::log
    absl::status
    absl::statusor
    absl::synchronization
    absl::time
)

# dsp

add_library(soir_dsp
    cpp/dsp/band_pass_filter.cc
    cpp/dsp/biquad_filter.cc
    cpp/dsp/chorus.cc
    cpp/dsp/comb_filter.cc
    cpp/dsp/delay.cc
    cpp/dsp/delayed_apf.cc
    cpp/dsp/high_pass_filter.cc
    cpp/dsp/high_shelving_filter.cc
    cpp/dsp/lfo.cc
    cpp/dsp/low_pass_filter.cc
    cpp/dsp/low_shelving_filter.cc
    cpp/dsp/lpf.cc
    cpp/dsp/modulated_delay.cc
    cpp/dsp/reverb.cc
    cpp/dsp/tools.cc
    cpp/dsp/two_band_shelving_filter.cc
)

target_include_directories(soir_dsp PUBLIC cpp)

target_link_libraries(soir_dsp
    soir_utils
    absl::log
    absl::status
    absl::statusor
)

# core

pybind11_add_module(_bindings
    cpp/bindings/bindings.cc
    cpp/bindings/logger.cc
    cpp/bindings/rt.cc
    cpp/bindings/soir.cc
    cpp/core/soir.cc
    cpp/rt/runtime.cc
)

target_include_directories(_bindings PRIVATE cpp)

target_link_libraries(_bindings PRIVATE
    soir_engine
    soir_utils
    absl::status
    absl::statusor
    absl::log
    absl::log_initialize
    absl::synchronization
    absl::time
)

target_compile_options(_bindings PRIVATE
    -Os
    -fvisibility=hidden
)

set_target_properties(_bindings PROPERTIES
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
    pybind11::embed
    gtest
    gtest_main
)

add_test(NAME CoreTest COMMAND core_test)

add_executable(engine_test
    cpp/tests/core/engine_test.cc
)

target_link_libraries(engine_test
    soir_engine
    sdk
    sdk_hosting
    pybind11::embed
    gtest
    gtest_main
)

add_test(NAME EngineTest COMMAND engine_test)

add_executable(inst_test
    cpp/tests/inst/sampler_test.cc
)

target_link_libraries(inst_test
    soir_inst
    pybind11::embed
    gtest
    gtest_main
)

add_test(NAME InstTest COMMAND inst_test)

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
