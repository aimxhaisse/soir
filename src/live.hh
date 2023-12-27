#pragma once

#include <absl/status/status.h>

#include "common/config.hh"

namespace maethstro {

constexpr const char* kVersion = "0.0.1-alpha.1";

class Live {
 public:
  static absl::Status Preamble();

  static absl::Status StandaloneMode(const Config& config);
  static absl::Status MatinMode(const Config& config);
  static absl::Status MidiMode(const Config& config);
  static absl::Status SoirMode(const Config& config);
};

}  // namespace maethstro
