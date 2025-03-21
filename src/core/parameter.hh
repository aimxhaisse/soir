#pragma once

#include <pybind11/stl.h>
#include <rapidjson/document.h>
#include <variant>

#include "core/common.hh"

namespace py = pybind11;

namespace soir {


class Controls;
class Control;


// This is meant to be used in Python bindings to map back the correct
// Python type so that we can have idempotent get_tracks/setup_tracks
// calls.
using ParameterRaw = std::variant<std::string,  // Name of the control if knob
                                  float  // Value of the parameter if constant
                                  >;

// Wrapper around a parameter that can either be controlled by a knob
// or set directly. This is meant to be initialized in rt bindings'
// code and used in DSP code to provide smooth interpolated values.
class Parameter {
 public:
  Parameter();
  Parameter(float v);

  float GetValue(SampleTick tick);
  void SetConstant(float value);
  void SetControl(Controls* controls, const std::string& value);
  ParameterRaw Raw() const;

  static Parameter FromPyDict(Controls* c, py::dict& p, const char* n);
  static Parameter FromJSON(Controls* c, rapidjson::Document& p,
                            const char* n);

 private:
  void Reset();

  enum class Type {
    CONSTANT,
    KNOB,
  };

  Type type_ = Type::CONSTANT;

  float constant_ = 0.0f;

  Controls* controls_ = nullptr;
  std::string controlName_;
  std::shared_ptr<Control> knob_ = nullptr;
};

}  // namespace soir
