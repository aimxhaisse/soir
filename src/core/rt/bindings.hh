#pragma once

#include <absl/status/status.h>
#include <optional>

namespace soir {

namespace dsp {
class Engine;
}  // namespace dsp

namespace rt {

class Engine;

namespace bindings {

absl::Status SetEngines(rt::Engine* rt, dsp::Engine* dsp);
void ResetEngines();
dsp::Engine* GetDsp();
rt::Engine* GetRt();

}  // namespace bindings
}  // namespace rt
}  // namespace soir
