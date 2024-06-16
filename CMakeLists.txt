# Neon.

cmake_minimum_required(VERSION 3.28.1)
project(
  neon
  LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ABSL_ENABLE_INSTALL ON)
set(Python_FIND_VIRTUALENV ONLY)

find_package(Python 3.12 COMPONENTS Interpreter Development REQUIRED)

add_subdirectory(deps/grpc)
add_subdirectory(deps/efsw)
add_subdirectory(deps/pybind11)
add_subdirectory(deps/httplib)
add_subdirectory(deps/libremidi)
add_subdirectory(deps/audiofile)
add_subdirectory(deps/rapidjson)
add_subdirectory(deps/rtaudio)

include(deps/grpc/third_party/protobuf/cmake/protobuf-generate.cmake)

find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

include_directories(${YAML_INCLUDE_DIRS})
include_directories(src)

add_subdirectory(src/agent)
add_subdirectory(src/utils)
add_subdirectory(src/proto)
add_subdirectory(src/core)

add_compile_definitions(NEON_VERSION=${VERSION})

add_executable(neon
  src/main.cc
)

target_link_libraries(
  neon
  neon_agent
  neon_core
  neon_utils
  neon_proto
  absl::base
  absl::log
  absl::status
  absl::flags
  absl::flags_parse
)
