#include <pybind11/pybind11.h>

#include "bindings/bind.hh"
#include "bindings/rt.hh"
#include "rt/runtime.hh"

namespace py = pybind11;

namespace soir {
namespace bindings {

void Bind::PyState(py::module_& m) {
  auto state = m.def_submodule("state", "Thread-safe engine state");

  state.def("get_snapshot_", []() -> std::string {
    return soir::rt::bindings::GetRt()->GetSnapshotJson();
  });
}

}  // namespace bindings
}  // namespace soir
