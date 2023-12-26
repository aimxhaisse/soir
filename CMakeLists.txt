cmake_minimum_required(VERSION 3.28.1)

project(
  maethstro
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(MAETHSTRO_VERSION=${VERSION})

add_subdirectory(deps/abseil)

add_executable(maethstro
  src/main.cc
)

target_link_libraries(maethstro
        absl::strings
        absl::log
        absl::status
)
