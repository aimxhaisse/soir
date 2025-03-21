#pragma once

#include <absl/status/status.h>

namespace soir {


// LFO that returns a value between [-1.0, 1.0].
class LFO {
 public:
  enum Type { SAW = 0, TRI = 1, SINE = 2 };

  struct Settings {
    Type type_ = SAW;
    float frequency_ = 0.0f;
  };

  LFO();

  absl::Status Init(const Settings& p);

  // Used to initialize the phase of the LFO, must be in the [0.0f,
  // 1.0f] range.
  void setPhase(float phase);

  // Returns a value between [-1.0, 1.0] and advances the internal
  // phase.
  float Render();

 private:
  Settings settings_;
  float lastPhase_ = 0.0f;
  float inc_ = 0.0f;
  float value_ = 0.0f;
};


}  // namespace soir
