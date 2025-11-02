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
          [](Soir& self, utils::Config& config) {
            auto status = self.Init(&config);
            if (!status.ok()) {
              throw std::runtime_error("Failed to initialize Soir: " +
                                       std::string(status.message()));
            }
          },
          py::arg("config"), "Initialize Soir with configuration")
      .def(
          "start",
          [](Soir& self) {
            auto status = self.Start();
            if (!status.ok()) {
              throw std::runtime_error("Failed to start Soir: " +
                                       std::string(status.message()));
            }
          },
          "Start the Soir engine")
      .def(
          "stop",
          [](Soir& self) {
            auto status = self.Stop();
            if (!status.ok()) {
              throw std::runtime_error("Failed to stop Soir: " +
                                       std::string(status.message()));
            }
          },
          "Stop the Soir engine")
      .def(
          "update_code",
          [](Soir& self, const std::string& code) {
            auto status = self.UpdateCode(code);
            if (!status.ok()) {
              throw std::runtime_error("Failed to update code: " +
                                       std::string(status.message()));
            }
          },
          py::arg("code"), "Update the live code");
}

}  // namespace bindings
}  // namespace soir
