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

}  // namespace soir
