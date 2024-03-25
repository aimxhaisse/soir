add_library(neon_core
  neon.cc
)

target_link_libraries(neon_core
  pybind11_headers
  pybind11::embed
  libremidi
)
