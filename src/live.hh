#pragma once

#include <absl/status/status.h>

#include "common/config.hh"

namespace maethstro {

constexpr const char* kVersion = "0.0.1-alpha.1";

class Live {
 public:
  static absl::Status Preamble();
  static absl::Status Standalone(const Config& config);
  static absl::Status Matin(const Config& config);
  static absl::Status Midi(const Config& config);
  static absl::Status Soir(const Config& config);
};

}  // namespace maethstro
