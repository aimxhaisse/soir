# Meta.

cmake_minimum_required(VERSION 3.28.1)
include(FetchContent)
project(
  maethstro
  LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 20)

# Deps.

add_subdirectory(deps/abseil)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
find_package(yaml-cpp REQUIRED)
include_directories(${YAML_INCLUDE_DIRS})

# Build.

enable_testing()
add_compile_definitions(MAETHSTRO_VERSION=${VERSION})

add_library(maethstro_matin
  src/matin/matin.cc
)

add_library(maethstro_common
  src/common/config.cc
)

target_link_libraries(maethstro_common
  ${YAML_CPP_LIBRARIES}
  absl::strings
  absl::log
  absl::status  
  absl::statusor
)

add_executable(maethstro_common_test
  src/common/config_test.cc
)

target_link_libraries(
  maethstro_common_test
  maethstro_common
  GTest::gtest_main
)

add_executable(maethstro
  src/main.cc
)

target_link_libraries(maethstro
  maethstro_common
  maethstro_matin
  absl::log
  absl::status
)
