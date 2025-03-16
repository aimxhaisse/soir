#include <absl/log/log.h>

#include "core/dsp/controls.hh"

#include "core/parameter.hh"

namespace soir {

float Parameter::GetValue(SampleTick tick) {
  if (type_ == Type::KNOB) {
    if (!knob_) {
      // Because knob creation happens asynchronously via MIDI events
      // that are scheduled from the RT thread, there is a possibility
      // a control is defined in Python but not yet reflected in the
      // DSP code. We lazily try to get it here, which has a cost
      // (O(log(n)) but should only happen at worst once, because MIDI
      // events are processed prior to rendering in the DSP thread.
      knob_ = controls_->GetControl(controlName_);
    }
    if (knob_) {
      return knob_->GetValue(tick);
    }
  }

  return constant_;
}

void Parameter::Reset() {
  type_ = Type::CONSTANT;
  constant_ = 0.0f;
  controlName_.clear();
  controls_ = nullptr;
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
  controls_ = controls;
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
