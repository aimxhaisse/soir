cmake_minimum_required(VERSION 3.15)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")

project(soir_core)

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
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
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

FetchContent_MakeAvailable(pybind11 yaml-cpp abseil-cpp googletest)

# utils

add_library(soir_utils
    src/utils/config.cc
    src/utils/config.hh
)

target_include_directories(soir_utils PUBLIC src)

target_link_libraries(soir_utils
    yaml-cpp::yaml-cpp
    absl::base
    absl::strings
    absl::status
    absl::log
)

# core

pybind11_add_module(_core
    src/bindings/bindings.cc
    src/bindings/start.cc
    src/core/soir.cc
)

target_include_directories(_core PRIVATE src)

target_link_libraries(_core PRIVATE
    soir_utils
)

target_compile_options(_core PRIVATE
    -Os
    -fvisibility=hidden
)

set_target_properties(_core PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
)

enable_testing()

add_executable(config_test
    tests/utils/config_test.cc
)

target_link_libraries(config_test
    soir_utils
    gtest
    gtest_main
    absl::statusor
)

add_test(NAME ConfigTest COMMAND config_test)
