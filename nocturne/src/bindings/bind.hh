#pragma once

#include <pybind11/pybind11.h>

namespace soir {
namespace bindings {

class Bind {
 public:
  static void Start(pybind11::module_& m);
};

}  // namespace bindings
}  // namespace soir
