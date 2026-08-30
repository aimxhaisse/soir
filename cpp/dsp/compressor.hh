#pragma once

#include <utility>

#include "core/common.hh"

namespace soir {
namespace dsp {

// A stereo compressor with a mono linked envelope, following the
// classic feedforward design: the envelope tracks the key signal, the
// computed gain is applied to the input.
//
// The key can be the input itself (plain in-line compression) or an
// external signal (sidechain compression), the caller decides which
// samples it feeds.
//
// The gain reduction uses a parabolic soft knee (C1-continuous)
// centered on the threshold, with a knob-able width in dB. A knee of
// 0.0 gives a hard knee.
class Compressor {
 public:
  struct Parameters {
    float threshold_ = 0.25f;  // Linear amplitude in [0.0, 1.0].
    float ratio_ = 4.0f;       // Compression ratio in [1.0, 20.0].
    float attack_ = 0.005f;    // Attack time in seconds in [0.0005, 0.5].
    float release_ = 0.15f;    // Release time in seconds in [0.01, 2.0].
    float knee_ = 6.0f;        // Soft knee width in dB in [0.0, 12.0].
    float makeup_ = 1.0f;      // Makeup gain in [0.0, 4.0].
    float wet_ = 1.0f;         // Dry/wet blend in [0.0, 1.0].
  };

  Compressor();

  // Reset the internal state (envelope and gain reduction).
  void Reset();

  // Recompute the attack/release smoothing coefficients. Safe to call
  // at any time, in particular per sample with interpolated values.
  void FastUpdate(const Parameters& params);

  // Process one sample pair: the envelope follows the key signal, the
  // gain is applied to the input. Returns the processed stereo sample.
  std::pair<float, float> Process(float key_l, float key_r, float in_l,
                                  float in_r);

  // Gain reduction applied at the last processed sample, in dB
  // (positive = attenuating).
  float GainReduction() const;

 private:
  Parameters params_;

  // One-pole envelope follower state (linear amplitude).
  float env_ = 0.0f;
  float attack_alpha_ = 0.0f;
  float release_alpha_ = 0.0f;

  // Gain reduction of the last processed sample, in dB.
  float gain_reduction_db_ = 0.0f;
};

}  // namespace dsp
}  // namespace soir
