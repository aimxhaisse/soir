#include "core/level_meter.hh"

#include <algorithm>
#include <cmath>

namespace soir {

LevelMeter::LevelMeter() {
  // Calculate decay coefficient: decay to ~37% (1/e) in kPeakDecayTime seconds.
  // Per-block decay: we process kBlockSize samples at kSampleRate.
  float blocks_per_second = static_cast<float>(kSampleRate) / kBlockSize;
  decay_coeff_ = std::exp(-1.0f / (kPeakDecayTime * blocks_per_second));
}

void LevelMeter::Process(const float* left, const float* right,
                         std::size_t size) {
  float inst_peak_l = 0.0f;
  float inst_peak_r = 0.0f;
  float sum_sq_l = 0.0f;
  float sum_sq_r = 0.0f;

  for (std::size_t i = 0; i < size; ++i) {
    inst_peak_l = std::max(inst_peak_l, std::abs(left[i]));
    inst_peak_r = std::max(inst_peak_r, std::abs(right[i]));
    sum_sq_l += left[i] * left[i];
    sum_sq_r += right[i] * right[i];
  }

  // Peak hold with decay: take max of decayed previous peak and new peak.
  float prev_peak_l = peak_left_.load(std::memory_order_relaxed);
  float prev_peak_r = peak_right_.load(std::memory_order_relaxed);

  float new_peak_l = std::max(inst_peak_l, prev_peak_l * decay_coeff_);
  float new_peak_r = std::max(inst_peak_r, prev_peak_r * decay_coeff_);

  peak_left_.store(new_peak_l, std::memory_order_relaxed);
  peak_right_.store(new_peak_r, std::memory_order_relaxed);

  // RMS is instantaneous per block.
  float fsize = static_cast<float>(size);
  rms_left_.store(std::sqrt(sum_sq_l / fsize), std::memory_order_relaxed);
  rms_right_.store(std::sqrt(sum_sq_r / fsize), std::memory_order_relaxed);
}

Levels LevelMeter::GetLevels() const {
  return Levels{
      .peak_left = peak_left_.load(std::memory_order_relaxed),
      .peak_right = peak_right_.load(std::memory_order_relaxed),
      .rms_left = rms_left_.load(std::memory_order_relaxed),
      .rms_right = rms_right_.load(std::memory_order_relaxed),
  };
}

}  // namespace soir
