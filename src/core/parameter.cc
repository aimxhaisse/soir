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
}

void Parameter::SetControl(dsp::Controls* controls, const std::string& name) {
  knob_ = controls->GetControl(name);

  if (knob_ == nullptr) {
    constant_ = 0.0f;
    type_ = Type::CONSTANT;
  } else {
    type_ = Type::KNOB;
  }
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

}  // namespace soir
