#pragma once

#include <absl/status/status.h>

#include "common/config.hh"

namespace maethstro {

constexpr const char* kVersion = "0.0.1-alpha.1";

class Live {
 public:
  static absl::Status Preamble();

  static absl::Status RunStandalone(const Config& config);
  static absl::Status RunMatin(const Config& config);
  static absl::Status RunMidi(const Config& config);
  static absl::Status RunSoir(const Config& config);
};

}  // namespace maethstro
