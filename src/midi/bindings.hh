#pragma once

#include <absl/status/status.h>
#include <optional>

namespace maethstro {
namespace midi {

class Engine;

namespace bindings {

absl::Status SetEngine(Engine* engine);
void ResetEngine();

struct PyTrack {
  std::string instrument;
  int channel = 0;

  std::optional<bool> muted;
  std::optional<int> volume;
  std::optional<int> pan;
};

}  // namespace bindings
}  // namespace midi
}  // namespace maethstro
