#include "bindings/bind.hh"

#include <pybind11/pybind11.h>

#include "absl/log/log.h"
#include "core/soir.hh"
#include "utils/config.hh"

namespace py = pybind11;

namespace soir {
namespace bindings {

static bool Start(const std::string& config_path) {
  auto config_result = utils::Config::LoadFromPath(config_path);
  if (!config_result.ok()) {
    LOG(ERROR) << "Failed to load config: " << config_result.status();
    return false;
  }

  Soir soir;
  auto init_status = soir.Init(config_result->get());
  if (!init_status.ok()) {
    LOG(ERROR) << "Failed to initialize Soir: " << init_status;
    return false;
  }

  auto start_status = soir.Start();
  if (!start_status.ok()) {
    LOG(ERROR) << "Failed to start Soir: " << start_status;
    return false;
  }

  return true;
}

void Bind::Start(py::module_& m) {
  m.def("Start", &soir::bindings::Start, py::arg("config_path"),
        "Start Soir engine with configuration file");
}

}  // namespace bindings
}  // namespace soir
