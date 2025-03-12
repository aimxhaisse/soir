#include <absl/log/log.h>

#include "core/dsp/controls.hh"

#include "core/parameter.hh"

namespace soir {

float Parameter::GetValue(SampleTick tick) {
  if (type_ == Type::KNOB && knob_) {
    return knob_->GetValue(tick);
  }

  return constant_;
}

void Parameter::Reset() {
  type_ = Type::CONSTANT;
  constant_ = 0.0f;
  controlName_.clear();

  // TODO: here if we handle ref counted knobs, we should somehow
  // decrement our reference.
  knob_ = nullptr;
}

void Parameter::SetConstant(float constant) {
  Reset();

  type_ = Type::CONSTANT;
  constant_ = constant;
}

void Parameter::SetControl(dsp::Controls* controls, const std::string& name) {
  Reset();

  knob_ = controls->GetControl(name);
  type_ = Type::KNOB;
  controlName_ = name;
}

Parameter Parameter::FromPyDict(dsp::Controls* c, py::dict& p, const char* n) {
  Parameter param;
  py::object ref = p[n];

  // Here we assume the object is a control and has a name attribute. We might
  // want to improve this at some point if we have to handle other types of
  // objects as parameters.
  if (py::isinstance<py::object>(ref) && py::hasattr(ref, "name_")) {
    param.SetControl(c, py::getattr(ref, "name_").cast<std::string>());
  } else {
    param.SetConstant(ref.cast<float>());
  }

  return param;
}

ParameterRaw Parameter::Raw() const {
  switch (type_) {
    case Type::KNOB:
      return controlName_;

    default:
      return constant_;
  }
}

}  // namespace soir
