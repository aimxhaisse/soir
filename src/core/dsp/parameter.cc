#include "core/dsp/parameter.hh"

namespace soir {
namespace dsp {

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

void Parameter::SetKnob(Controls* controls, const std::string& name) {
  knob_ = controls->GetControl(name);

  if (knob_ == nullptr) {
    constant_ = 0.0f;
    type_ = Type::CONSTANT;
  } else {
    type_ = Type::KNOB;
  }
}

}  // namespace dsp
}  // namespace soir
