#pragma once

#include <atomic>
#include <cstddef>

#include "core/common.hh"

namespace soir {

struct Levels {
  float peak_left = 0.0f;
  float peak_right = 0.0f;
  float rms_left = 0.0f;
  float rms_right = 0.0f;
};

class LevelMeter {
 public:
  // Decay time in seconds (0.3s for snappy response).
  static constexpr float kPeakDecayTime = 0.3f;

  LevelMeter();

  // Process a stereo audio buffer and update levels.
  void Process(const float* left, const float* right, std::size_t size);

  // Get current levels (thread-safe).
  Levels GetLevels() const;

 private:
  // Decay coefficient calculated from decay time and sample rate.
  float decay_coeff_;

  // Atomic storage for thread-safe access.
  std::atomic<float> peak_left_{0.0f};
  std::atomic<float> peak_right_{0.0f};
  std::atomic<float> rms_left_{0.0f};
  std::atomic<float> rms_right_{0.0f};
};

}  // namespace soir
