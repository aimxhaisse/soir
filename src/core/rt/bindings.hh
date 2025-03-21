#pragma once

#include <absl/status/status.h>
#include <optional>

namespace soir {

namespace engine {
class Engine;
}  // namespace engine

namespace rt {

class Runtime;

namespace bindings {

absl::Status SetEngines(rt::Runtime* rt, engine::Engine* dsp);
void ResetEngines();
engine::Engine* GetDsp();
rt::Runtime* GetRt();

}  // namespace bindings
}  // namespace rt
}  // namespace soir
