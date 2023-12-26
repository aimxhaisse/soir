#pragma once

#include "absl/status/status.h"

namespace maethstro {

class Matin {
 public:
  Matin();
  ~Matin();

  absl::Status Run();
};

}  // namespace maethstro
