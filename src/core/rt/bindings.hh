#pragma once

#include <absl/status/status.h>
#include <optional>

namespace soir {

namespace engine {
class Engine;
}  // namespace engine

namespace rt {

class Engine;

namespace bindings {

absl::Status SetEngines(rt::Engine* rt, engine::Engine* dsp);
void ResetEngines();
engine::Engine* GetDsp();
rt::Engine* GetRt();

}  // namespace bindings
}  // namespace rt
}  // namespace soir
