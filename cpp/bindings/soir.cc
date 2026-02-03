#include "core/soir.hh"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "absl/log/log.h"
#include "bindings/bind.hh"
#include "utils/config.hh"

namespace py = pybind11;

namespace soir {
namespace bindings {

void Bind::PySoir(py::module_& m) {
  py::class_<Soir>(m, "Soir")
      .def(py::init<>())
      .def(
          "init",
          [](Soir& self, const std::string& cfg_path) {
            py::gil_scoped_release release;
            auto status = self.Init(cfg_path);
            if (!status.ok()) {
              LOG(ERROR) << "Failed to initialize Soir: " << status.message();
              return false;
            }

            return true;
          },
          py::arg("config"), "Initialize Soir with configuration")
      .def(
          "start",
          [](Soir& self) {
            py::gil_scoped_release release;
            auto status = self.Start();
            if (!status.ok()) {
              LOG(ERROR) << "Failed to start Soir: " << status.message();
              return false;
            }

            return true;
          },
          "Start the Soir engine")
      .def(
          "stop",
          [](Soir& self) {
            py::gil_scoped_release release;
            auto status = self.Stop();
            if (!status.ok()) {
              LOG(ERROR) << "Failed to stop Soir: " << status.message();
              return false;
            }

            return true;
          },
          "Stop the Soir engine")
      .def(
          "update_code",
          [](Soir& self, const std::string& code) {
            py::gil_scoped_release release;
            auto status = self.UpdateCode(code);
            if (!status.ok()) {
              LOG(ERROR) << "Failed to update code: " << status.message();
              return false;
            }

            return true;
          },
          py::arg("code"), "Update the live code");
}

}  // namespace bindings
}  // namespace soir
