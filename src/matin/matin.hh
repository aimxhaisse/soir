#pragma once

#include "absl/status/status.h"

#include "common/config.hh"

namespace maethstro {

struct MatinSettings {
  std::string directory;
  std::string midi_grpc_host;
  int midi_grpc_port;
};

class Matin {
 public:
  Matin();
  ~Matin();

  absl::Status Init(const Config& config);
  absl::Status Run();

 private:
  MatinSettings settings_;
};

}  // namespace maethstro
