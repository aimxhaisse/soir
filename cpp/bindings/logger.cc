#include "utils/logger.hh"

#include <pybind11/pybind11.h>

#include "absl/log/log.h"
#include "bindings/bind.hh"

namespace py = pybind11;

namespace soir {
namespace bindings {

void Bind::PyLogger(py::module_& m) {
  auto logging = m.def_submodule("logging", "Logging utilities");

  logging.def(
      "init",
      [](const std::string& log_dir, size_t max_files, bool verbose) {
        auto status =
            utils::Logger::Instance().Init(log_dir, max_files, verbose);
        if (!status.ok()) {
          throw std::runtime_error("Failed to initialize logger: " +
                                   std::string(status.message()));
        }
      },
      py::arg("log_dir"), py::arg("max_files") = 25, py::arg("verbose") = false,
      "Initialize logger with directory, optional max files, and verbose flag");

  logging.def(
      "shutdown",
      []() {
        auto status = utils::Logger::Instance().Shutdown();
        if (!status.ok()) {
          throw std::runtime_error("Failed to shutdown logger: " +
                                   std::string(status.message()));
        }
      },
      "Shutdown the logger");

  logging.def(
      "info", [](const std::string& msg) { LOG(INFO) << msg; },
      py::arg("message"), "Log an info message");

  logging.def(
      "warning", [](const std::string& msg) { LOG(WARNING) << msg; },
      py::arg("message"), "Log a warning message");

  logging.def(
      "error", [](const std::string& msg) { LOG(ERROR) << msg; },
      py::arg("message"), "Log an error message");
}

}  // namespace bindings
}  // namespace soir
