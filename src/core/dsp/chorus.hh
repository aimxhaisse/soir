#pragma once

#include <absl/status/status.h>
#include <utility>

#include "core/dsp.hh"
#include "modulated_delay.hh"

namespace soir {
namespace dsp {

// A Stereo Chorus based on the design of the Korg LCR.
class Chorus {
 public:
  struct Parameters {
    float time_ = 0.5f;  // coefficient in [0.0, 1.0]
    float depth_ = 0;    // coefficient in [0.0, 1.0]
    float rate_ = 0.5f;
  };

  Chorus();

  // Initialize with the given parameters
  absl::Status Init(const Parameters& p);

  // Fast update to parameters that don't require full reinitialization
  absl::Status FastUpdate(const Parameters& p);

  // Process a stereo sample and return a stereo sample
  std::pair<float, float> Render(float lxn, float rxn);

  // Reset the internal state
  void Reset();

 private:
  void InitFromParameters();

  Parameters params_;

  ModulatedDelay::Parameters left_params_;
  ModulatedDelay::Parameters center_params_;
  ModulatedDelay::Parameters right_params_;

  ModulatedDelay left_;
  ModulatedDelay center_;
  ModulatedDelay right_;
};

inline bool operator!=(const Chorus::Parameters& lhs,
                       const Chorus::Parameters& rhs) {
  return std::tie(lhs.time_, lhs.depth_, lhs.rate_) !=
         std::tie(rhs.time_, rhs.depth_, rhs.rate_);
}

}  // namespace dsp
}  // namespace soir
