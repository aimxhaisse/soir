#include "core/parameter.hh"

#include <absl/log/log.h>

#include "core/controls.hh"

namespace soir {

Parameter::Parameter() { Reset(); }

Parameter::Parameter(float constant, float min, float max) {
  SetConstant(constant);

  min_ = min;
  max_ = max;
}

Parameter::Parameter(float constant) { SetConstant(constant); }

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
      return Clip(knob_->GetValue(tick));
    }
  }

  return Clip(constant_);
}

float Parameter::Clip(float v) const {
  float ret = v;

  if (min_.has_value()) {
    ret = std::max(ret, min_.value());
  }

  if (max_.has_value()) {
    ret = std::min(ret, max_.value());
  }

  return ret;
}

void Parameter::Reset() {
  type_ = Type::CONSTANT;
  constant_ = 0.0f;
  controlName_.clear();
  controls_ = nullptr;
}

void Parameter::SetRange(float min, float max) {
  min_ = min;
  max_ = max;
}

void Parameter::SetConstant(float constant) {
  Reset();

  type_ = Type::CONSTANT;
  constant_ = constant;
}

void Parameter::SetControl(Controls* controls, const std::string& name) {
  Reset();

  knob_ = controls->GetControl(name);
  type_ = Type::KNOB;
  controlName_ = name;
  controls_ = controls;
}

Parameter Parameter::FromPyDict(Controls* c, py::dict& p, const char* n) {
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

  // Bool not handled yet.

  return param;
}

Parameter Parameter::FromJSON(Controls* c, rapidjson::Document& p,
                              const char* n) {
  Parameter param;

  if (!p.HasMember(n)) {
    return param;
  }

  // Unsafe, we assume we always have a value here.
  const rapidjson::Value& ref = p[n];

  if (ref.IsString()) {
    param.SetControl(c, ref.GetString());
  } else if (ref.IsUint()) {
    param.SetConstant(static_cast<float>(ref.GetUint()));
  } else if (ref.IsInt()) {
    param.SetConstant(static_cast<float>(ref.GetInt()));
  } else if (ref.IsUint64()) {
    param.SetConstant(static_cast<float>(ref.GetUint64()));
  } else if (ref.IsInt64()) {
    param.SetConstant(static_cast<float>(ref.GetInt64()));
  } else {
    param.SetConstant(static_cast<float>(ref.GetDouble()));
  }

  // Bool not handled yet.

  return param;
}

ParameterRaw Parameter::Raw() const {
  switch (type_) {
    case Type::KNOB:
      return controlName_;

    default:
      return Clip(constant_);
  }
}

}  // namespace soir
