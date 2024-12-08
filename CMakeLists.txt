# Soir.

cmake_minimum_required(VERSION 3.28.1)
project(
  soir
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
add_subdirectory(deps/sdl)
add_subdirectory(deps/yaml)
add_subdirectory(deps/ogg)
add_subdirectory(deps/vorbis)

include(deps/grpc/third_party/protobuf/cmake/protobuf-generate.cmake)

find_package(GTest REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(src)

add_subdirectory(src/agent)
add_subdirectory(src/utils)
add_subdirectory(src/proto)
add_subdirectory(src/core)

add_compile_definitions(SOIR_VERSION=${VERSION})

add_executable(soir
  src/main.cc
)

target_link_libraries(
  soir
  soir_agent
  soir_core
  soir_utils
  soir_proto
  absl::base
  absl::log
  absl::status
  absl::flags
  absl::flags_parse
)
