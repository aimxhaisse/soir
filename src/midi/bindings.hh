#pragma once

#include <absl/status/status.h>

namespace maethstro {

class Engine;

namespace bindings {

absl::Status SetEngine(Engine* engine);
void ResetEngine();

}  // namespace bindings
}  // namespace maethstro
