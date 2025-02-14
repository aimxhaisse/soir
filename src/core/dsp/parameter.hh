#pragma once

#include "core/common.hh"
#include "core/dsp/controls.hh"

namespace soir {
namespace dsp {

// Wrapper around a parameter that can either be controlled by a knob
// or set directly.
class Parameter {
 public:
  Parameter() = default;

  float GetValue(SampleTick tick);

  void SetConstant(float value);
  void SetControl(Controls* controls, const std::string& value);

 private:
  enum class Type {
    CONSTANT,
    KNOB,
  };

  Type type_ = Type::CONSTANT;
  float constant_ = 0.0f;
  Control* knob_ = nullptr;
};

}  // namespace dsp
}  // namespace soir
