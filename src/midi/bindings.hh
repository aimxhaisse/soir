#pragma once

#include <absl/status/status.h>

namespace maethstro {
namespace midi {

class Engine;

namespace bindings {

absl::Status SetEngine(Engine* engine);
void ResetEngine();

struct PyTrack {
  std::string instrument;
  int channel = 0;
  bool muted = false;
  int volume = 127;
  int pan = 64;
};

}  // namespace bindings
}  // namespace midi
}  // namespace maethstro
