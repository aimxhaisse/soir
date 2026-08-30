#include "dsp/compressor.hh"

#include <cmath>

namespace soir {
namespace dsp {

Compressor::Compressor() { FastUpdate(params_); }

void Compressor::Reset() {
  env_ = 0.0f;
  gain_reduction_db_ = 0.0f;
}

void Compressor::FastUpdate(const Parameters& params) {
  params_ = params;

  const float sr = static_cast<float>(kSampleRate);
  attack_alpha_ = 1.0f - std::exp(-1.0f / (params_.attack_ * sr));
  release_alpha_ = 1.0f - std::exp(-1.0f / (params_.release_ * sr));
}

std::pair<float, float> Compressor::Process(float key_l, float key_r,
                                            float in_l, float in_r) {
  // Envelope follower (linear domain), mono linked.
  const float key = std::max(std::fabs(key_l), std::fabs(key_r));
  const float alpha = key > env_ ? attack_alpha_ : release_alpha_;
  env_ += (key - env_) * alpha;

  // Gain reduction (dB domain, parabolic soft knee).
  const float in_db = 20.0f * std::log10(std::max(env_, 1e-6f));
  const float t_db = 20.0f * std::log10(std::max(params_.threshold_, 1e-6f));

  float y_db;
  const float knee = params_.knee_;
  if (knee <= 0.0f) {
    y_db = in_db <= t_db ? in_db : t_db + (in_db - t_db) / params_.ratio_;
  } else {
    const float low = t_db - knee * 0.5f;
    const float high = t_db + knee * 0.5f;
    if (in_db <= low) {
      y_db = in_db;
    } else if (in_db >= high) {
      y_db = t_db + (in_db - t_db) / params_.ratio_;
    } else {
      // Parabolic blend: matches value and slope of both branches at
      // the band edges (C1-continuous).
      const float d = in_db - low;
      y_db = in_db + (1.0f / params_.ratio_ - 1.0f) * (d * d) / (2.0f * knee);
    }
  }

  gain_reduction_db_ = in_db - y_db;

  const float gain = std::pow(10.0f, -gain_reduction_db_ / 20.0f);
  const float wet_gain = gain * params_.makeup_ * params_.wet_;
  const float dry_gain = 1.0f - params_.wet_;

  return {in_l * (wet_gain + dry_gain), in_r * (wet_gain + dry_gain)};
}

float Compressor::GainReduction() const { return gain_reduction_db_; }

}  // namespace dsp
}  // namespace soir
