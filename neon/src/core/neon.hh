#pragma once

#include "core/dsp/engine.hh"
#include "utils/config.hh"

namespace neon {

class Neon {
 public:
  Neon();
  ~Neon();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  std::unique_ptr<dsp::Engine> dsp_;
};

}  // namespace neon
