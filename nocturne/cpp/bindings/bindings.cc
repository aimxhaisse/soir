#include <pybind11/pybind11.h>

#include "bindings/bind.hh"

namespace py = pybind11;

PYBIND11_MODULE(_core, m, py::mod_gil_not_used()) {
  m.doc() = "Soir C++ core module";

  soir::bindings::Bind::PySoir(m);
}
