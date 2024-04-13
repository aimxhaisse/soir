#pragma once

#include <absl/status/status.h>
#include <optional>

namespace neon {

namespace dsp {
class Engine;
}  // namespace dsp

namespace rt {

class Engine;

namespace bindings {

absl::Status SetEngines(rt::Engine* rt, dsp::Engine* dsp);
void ResetEngines();

struct PyTrack {
  std::string instrument;
  int channel = 0;

  std::optional<bool> muted;
  std::optional<int> volume;
  std::optional<int> pan;
};

}  // namespace bindings
}  // namespace rt
}  // namespace neon
