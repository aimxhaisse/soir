#pragma once

#include <absl/status/status.h>

#include "common/config.hh"

namespace maethstro {

constexpr const char* kVersion = "0.0.1-alpha.1";

class Live {
 public:
  static absl::Status Preamble();
  static absl::Status Standalone();
  static absl::Status Matin();
  static absl::Status Midi();
  static absl::Status Soir();
};

}  // namespace maethstro
