#include <pybind11/pybind11.h>

#include "bindings/bind.hh"

namespace py = pybind11;

PYBIND11_MODULE(_bindings, m, py::mod_gil_not_used()) {
  m.doc() = "Soir C++ bindings module";

  soir::bindings::Bind::PyLogger(m);
  soir::bindings::Bind::PySoir(m);
  soir::bindings::Bind::PyRt(m);
}
