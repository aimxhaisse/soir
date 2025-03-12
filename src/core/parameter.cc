#include "core/dsp/controls.hh"

#include "core/parameter.hh"

namespace soir {

float Parameter::GetValue(SampleTick tick) {
  switch (type_) {
    case Type::KNOB:
      return knob_->GetValue(tick);
    case Type::CONSTANT:
      return constant_;
  }
}

void Parameter::SetConstant(float constant) {
  type_ = Type::CONSTANT;
  constant_ = constant;

  // TODO: here if we handle ref counted knobs, we should somehow
  // decrement our reference.
  knob_ = nullptr;
  controlName_.clear();
}

void Parameter::SetControl(dsp::Controls* controls, const std::string& name) {
  knob_ = controls->GetControl(name);

  if (knob_ == nullptr) {
    SetConstant(0.0f);
    return;
  }

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
