#pragma once

#include <absl/status/status.h>

namespace maethstro {

class Midi {
 public:
  Midi();
  ~Midi();

  absl::Status Init();
  absl::Status Run();
};

}  // namespace maethstro
