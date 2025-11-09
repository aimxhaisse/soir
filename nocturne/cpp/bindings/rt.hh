#pragma once

#include <absl/status/status.h>

namespace soir {

class Engine;

namespace rt {

class Runtime;

namespace bindings {

absl::Status SetEngines(rt::Runtime* rt, Engine* dsp);
void ResetEngines();
Engine* GetDsp();
rt::Runtime* GetRt();

}  // namespace bindings
}  // namespace rt
}  // namespace soir
