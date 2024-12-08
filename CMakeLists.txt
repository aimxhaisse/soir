# Soir.

include(ExternalProject)

cmake_minimum_required(VERSION 3.28.1)

project(
  soir
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python 3.13 COMPONENTS Interpreter Development REQUIRED)

# Dependencies.

set(ABSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/absl_install)
set(PROTOBUF_INSTALL_DIR ${CMAKE_BINARY_DIR}/protobuf_install)
set(GRPC_INSTALL_DIR ${CMAKE_BINARY_DIR}/grpc_install)
set(EFSW_INSTALL_DIR ${CMAKE_BINARY_DIR}/efsw_install)
set(PYBIND11_INSTALL_DIR ${CMAKE_BINARY_DIR}/pybind11_install)
set(HTTPLIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/httplib_install)
set(LIBREMIDI_INSTALL_DIR ${CMAKE_BINARY_DIR}/libremidi_install)
set(AUDIOFILE_INSTALL_DIR ${CMAKE_BINARY_DIR}/audiofile_install)
set(RAPIDJSON_INSTALL_DIR ${CMAKE_BINARY_DIR}/rapidjson_install)
set(SDL_INSTALL_DIR ${CMAKE_BINARY_DIR}/sdl_install)
set(YAMLCPP_INSTALL_DIR ${CMAKE_BINARY_DIR}/yamlcpp_install)
set(OGG_INSTALL_DIR ${CMAKE_BINARY_DIR}/ogg_install)
set(VORBIS_INSTALL_DIR ${CMAKE_BINARY_DIR}/vorbis_install)
set(GOOGLETEST_INSTALL_DIR ${CMAKE_BINARY_DIR}/googletest_install)

ExternalProject_Add(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240722.0
  PREFIX ${CMAKE_BINARY_DIR}/absl
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ABSL_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG v1.60.0
  PREFIX ${CMAKE_BINARY_DIR}/grpc
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
  GIT_TAG 1.3.1
  PREFIX ${CMAKE_BINARY_DIR}/efsw
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EFSW_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.12.0
  PREFIX ${CMAKE_BINARY_DIR}/pybind11
  CMAKE_ARGS -DPYBIND11_FINDPYTHON=ON
             -DPYBIND11_PYTHON_VERSION=3.13
	     -DCMAKE_INSTALL_PREFIX=${PYBIND11_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)    

ExternalProject_Add(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.14.3
  PREFIX ${CMAKE_BINARY_DIR}/httplib
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${HTTPLIB_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  libremidi
  GIT_REPOSITORY https://github.com/jcelerier/libremidi.git
  GIT_TAG v4.5.0
  PREFIX ${CMAKE_BINARY_DIR}/libremidi
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBREMIDI_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  audiofile
  GIT_REPOSITORY https://github.com/adamstark/AudioFile.git
  GIT_TAG 1.1.2
  PREFIX ${CMAKE_BINARY_DIR}/audiofile
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${AUDIOFILE_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)   

ExternalProject_Add(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
  GIT_TAG v1.1.0
  PREFIX ${CMAKE_BINARY_DIR}/rapidjson
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${RAPIDJSON_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.7
  PREFIX ${CMAKE_BINARY_DIR}/sdl
  CMAKE_ARGS -DSDL_STATIC=ON
             -DCMAKE_INSTALL_PREFIX=${SDL_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
  PREFIX ${CMAKE_BINARY_DIR}/yaml-cpp
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${YAMLCPP_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  ogg
  GIT_REPOSITORY https://github.com/xiph/ogg.git
  GIT_TAG v1.3.5
  PREFIX ${CMAKE_BINARY_DIR}/ogg
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${OGG_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)   

ExternalProject_Add(
  vorbis
  GIT_REPOSITORY https://github.com/xiph/vorbis.git
  GIT_TAG v1.3.7
  PREFIX ${CMAKE_BINARY_DIR}/vorbis
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${VORBIS_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

ExternalProject_Add(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  PREFIX ${CMAKE_BINARY_DIR}/googletest
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GOOGLETEST_INSTALL_DIR}
             -DCMAKE_CXX_STANDARD=20
)

include_directories(
  ${ABSL_INSTALL_DIR}/include
  ${GRPC_INSTALL_DIR}/include
  ${PROTOBUF_INSTALL_DIR}/include
  ${EFSW_INSTALL_DIR}/include
  ${PYBIND11_INSTALL_DIR}/include
  ${HTTPLIB_INSTALL_DIR}/include
  ${LIBREMIDI_INSTALL_DIR}/include
  ${AUDIOFILE_INSTALL_DIR}/include
  ${RAPIDJSON_INSTALL_DIR}/include
  ${SDL_INSTALL_DIR}/include
  ${YAMLCPP_INSTALL_DIR}/include
  ${OGG_INSTALL_DIR}/include
  ${VORBIS_INSTALL_DIR}/include

  src
)

link_directories(
  ${ABSL_INSTALL_DIR}/lib
  ${PROTOBUF_INSTALL_DIR}/lib
  ${GRPC_INSTALL_DIR}/lib
  ${EFSW_INSTALL_DIR}/lib
  ${PYBIND11_INSTALL_DIR}/lib
  ${HTTPLIB_INSTALL_DIR}/lib
  ${LIBREMIDI_INSTALL_DIR}/lib
  ${AUDIOFILE_INSTALL_DIR}/lib
  ${RAPIDJSON_INSTALL_DIR}/lib
  ${SDL_INSTALL_DIR}/lib
  ${YAMLCPP_INSTALL_DIR}/lib
  ${OGG_INSTALL_DIR}/lib
  ${VORBIS_INSTALL_DIR}/lib
)

# Soir.

add_subdirectory(src/agent)
add_subdirectory(src/utils)
add_subdirectory(src/proto)
add_subdirectory(src/core)

add_compile_definitions(SOIR_VERSION=${VERSION})

add_executable(soir
  src/main.cc
)

add_dependencies(
  soir

  absl
)

target_link_libraries(
  soir
  soir_agent
  soir_core
  soir_utils
  soir_proto

  ${GRPC_INSTALL_DIR}/lib/libabsl_base.a
  ${GRPC_INSTALL_DIR}/lib/libabsl_log.a
  ${GRPC_INSTALL_DIR}/lib/libabsl_status.a
  ${GRPC_INSTALL_DIR}/lib/libabsl_flags.a
  ${GRPC_INSTALL_DIR}/lib/libabsl_flags_parse.a
)
