#pragma once

#include <absl/status/status.h>
#include <tuple>

#include "core/dsp.hh"

namespace soir {
namespace dsp {

// LFO that returns a value between [-1.0, 1.0].
class LFO {
 public:
  enum Type { SAW = 0, TRI = 1, SINE = 2 };

  struct Parameters {
    Type type_ = SAW;
    float frequency_ = 0.0f;
  };

  LFO();

  // Initialize LFO with the given parameters
  absl::Status Init(const Parameters& p);

  // Set the phase of the LFO, must be in the range [0.0, 1.0f].
  void SetPhase(float phase);

  // Returns a value between [-1.0, 1.0] and moves forward.
  float Render();

  // Reset the LFO.
  void Reset();

 private:
  // Init inc from parameters.
  void InitFromParameters();

  Parameters params_;
  float last_phase_ = 0.0f;
  float inc_ = 0.0f;
  float value_ = 0.0f;
};

inline bool operator!=(const LFO::Parameters& lhs, const LFO::Parameters& rhs) {
  return std::tie(lhs.type_, lhs.frequency_) !=
         std::tie(rhs.type_, rhs.frequency_);
}

}  // namespace dsp
}  // namespace soir
