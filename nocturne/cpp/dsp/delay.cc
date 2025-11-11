#include "dsp/delay.hh"

#include <absl/log/log.h>

#include <algorithm>
#include <cmath>

namespace soir {
namespace dsp {

namespace {

static constexpr int kLagrangeOrder = 4;

}  // namespace

Delay::Delay() {
  // Default construction, will be initialized with Init
}

void Delay::Init(const Parameters& p) {
  params_ = p;
  InitFromParameters();
}

void Delay::FastUpdate(const Parameters& p) {
  // Only update if max_ doesn't change, otherwise we need a full Init
  if (p.max_ == params_.max_) {
    params_.size_ = p.size_;
    params_.interpolation_ = p.interpolation_;
    return;
  }
  Init(p);
}

void Delay::InitFromParameters() {
  if (buffer_.size() != (params_.max_ + kLagrangeOrder)) {
    buffer_.resize(params_.max_ + kLagrangeOrder);
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    idx_ = 0;
  }
}

void Delay::Reset() {
  std::fill(buffer_.begin(), buffer_.end(), 0.0f);
  idx_ = 0;
}

float Delay::Render(float xn) {
  const float x = Read();
  Update(xn);
  return x;
}

float Delay::Read() const { return ReadAt(params_.size_); }

float Delay::ReadAt(float at) const {
  switch (params_.interpolation_) {
    case LAGRANGE:
      return ReadAtLagrange(at);

    case LINEAR:
    default:
      return ReadAtLinear(at);
  }
}

float Delay::ReadAtLinear(float at) const {
  const int low_bound = static_cast<int>(at);
  const float interpolation = at - static_cast<float>(low_bound);

  int aidx = (idx_ - low_bound);
  int bidx = aidx - 1;

  if (bidx < 0) {
    bidx += buffer_.size();
    if (aidx < 0) {
      aidx += buffer_.size();
    }
  }

  const float a = buffer_[aidx];
  const float b = buffer_[bidx];

  float res = a + interpolation * (b - a);

  return res;
}

float Delay::ReadAtLagrange(float at) const {
  // Fallback to linear interpolation if we don't have enough samples.
  if (params_.size_ < kLagrangeOrder) {
    return ReadAtLinear(at);
  }

  const int low_bound = static_cast<int>(at);
  const float interpolation = at - static_cast<float>(low_bound);

  int idx1 = (idx_ - low_bound);
  int idx2 = idx1 - 1;
  int idx3 = idx2 - 1;
  int idx4 = idx3 - 1;

  // Handle circular buffer.
  if (idx4 < 0) {
    idx4 += buffer_.size();
    if (idx3 < 0) {
      idx3 += buffer_.size();
      if (idx2 < 0) {
        idx2 += buffer_.size();
        if (idx1 < 0) {
          idx1 += buffer_.size();
        }
      }
    }
  }

  const float d1 = interpolation - 1.0f;
  const float d2 = interpolation - 2.0f;
  const float d3 = interpolation - 3.0f;

  const float c1 = -d1 * d2 * d3 / 6.0f;
  const float c2 = d2 * d3 / 2.0f;
  const float c3 = -d1 * d3 / 2.0f;
  const float c4 = d1 * d2 / 6.0f;

  float res = buffer_[idx1] * c1 +
              interpolation * (buffer_[idx2] * c2 + buffer_[idx3] * c3 +
                               buffer_[idx4] * c4);

  return res;
}

void Delay::Update(float xn) {
  buffer_[idx_] = xn;
  idx_ += 1;
  if (idx_ == buffer_.size()) {
    idx_ = 0;
  }
}

float Delay::Size() const { return params_.size_; }

}  // namespace dsp
}  // namespace soir
