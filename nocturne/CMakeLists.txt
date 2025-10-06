cmake_minimum_required(VERSION 3.15)
project(soir_core)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Release)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_core
    src/bindings.cc
    src/core.cc
)

target_include_directories(_core PRIVATE src)

target_compile_options(_core PRIVATE
    -Os
    -fvisibility=hidden
)

set_target_properties(_core PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
)
