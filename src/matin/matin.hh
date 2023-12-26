#pragma once

#include "absl/status/status.h"

namespace maethstro {

class Matin {
 public:
  Matin();
  ~Matin();

  absl::Status Init();
  absl::Status Run();
};

}  // namespace maethstro
