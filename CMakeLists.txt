# Soir.

include(ExternalProject)

cmake_minimum_required(VERSION 3.28.1)

set(PYBIND11_FINDPYTHON True)
set(BUILD_SHARED_LIBS OFF)

project(
  soir
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(LINK_OPTIONS "-w")

find_package(efsw CONFIG REQUIRED)
find_package(audiofile CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(libremidi CONFIG REQUIRED)
find_package(ogg CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(rapidjson CONFIG REQUIRED)
find_package(sdl2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(vorbis CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(GTest REQUIRED)

set(_GRPC_GRPCPP_UNSECURE gRPC::grpc++_unsecure)

find_program(_gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

include_directories(src)

add_subdirectory(src/agent)
add_subdirectory(src/utils)
add_subdirectory(src/proto)
add_subdirectory(src/core)

add_compile_definitions(SOIR_VERSION=${VERSION})

add_executable(
  soir

  src/main.cc
)

target_link_libraries(
  soir

  soir_agent
  soir_core
  soir_utils
  soir_proto

  absl::base
  absl::strings
  absl::flags
  absl::flags_parse
)

add_custom_command(
  TARGET soir
  POST_BUILD
  COMMAND strip $<TARGET_FILE:soir>
)
