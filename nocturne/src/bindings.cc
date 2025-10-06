#include <pybind11/pybind11.h>

#include "core.hh"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  m.doc() = "Soir C++ core module";
  m.def("hello_world", &soir::hello_world, "Returns a greeting from C++");
}
