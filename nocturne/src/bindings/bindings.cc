#include <pybind11/pybind11.h>

#include "bindings/bind.hh"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  m.doc() = "Soir C++ core module";
  soir::bindings::Bind::Start(m);
}
