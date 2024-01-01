#pragma once

#include <absl/status/status.h>

#include "common/config.hh"

namespace maethstro {
namespace soir {

class Soir {
 public:
  Soir();
  ~Soir();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();
};

}  // namespace soir
}  // namespace maethstro
