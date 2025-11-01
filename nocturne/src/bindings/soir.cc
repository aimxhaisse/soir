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
          [](Soir& self, const std::string& config_path) {
            auto config_result = utils::Config::LoadFromPath(config_path);
            if (!config_result.ok()) {
              throw std::runtime_error(
                  "Failed to load config: " +
                  std::string(config_result.status().message()));
            }
            auto status = self.Init(config_result->get());
            if (!status.ok()) {
              throw std::runtime_error("Failed to initialize Soir: " +
                                       std::string(status.message()));
            }
          },
          py::arg("config_path"), "Initialize Soir with configuration file")
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
