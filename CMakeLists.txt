# Meta.

cmake_minimum_required(VERSION 3.28.1)
project(
  maethstro
  LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(deps/abseil)

find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

include_directories(${YAML_INCLUDE_DIRS})
include_directories(src)

add_subdirectory(proto)
add_subdirectory(src/common)
add_subdirectory(src/matin)
add_subdirectory(src/midi)

add_compile_definitions(MAETHSTRO_VERSION=${VERSION})

add_executable(maethstro
  src/main.cc
  src/live.cc
)

target_link_libraries(
  maethstro
  absl::base
  absl::log
  absl::status
  absl::flags
  absl::flags_parse
  maethstro_common
  maethstro_matin
  maethstro_midi
)
