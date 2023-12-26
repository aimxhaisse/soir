# Meta.

cmake_minimum_required(VERSION 3.28.1)
project(
  maethstro
  LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 20)

find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

add_subdirectory(deps/abseil)
add_subdirectory(proto)
add_subdirectory(src/common)
add_subdirectory(src/matin)
add_subdirectory(src/midi)

include_directories(${YAML_INCLUDE_DIRS})

add_compile_definitions(MAETHSTRO_VERSION=${VERSION})

add_executable(maethstro
  src/main.cc
)

target_link_libraries(maethstro
  maethstro_common
  maethstro_matin
  maethstro_midi
  absl::log
  absl::status
)
