#pragma once

#include <absl/status/status.h>
#include <tuple>
#include <vector>

namespace soir {
namespace dsp {

// A simple delay, the size (number of samples to go back in time).
//
// Corresponding difference equation:
//
// y[n] = x[n - size]
//
// The minimum size of the delay is 1, trying to set the delay to 0
// will be ignored. We do so because there is no way to have a
// coherent interface with a 0 delay here.
class Delay {
 public:
  enum Interpolation { LINEAR = 0, LAGRANGE = 1 };

  struct Parameters {
    // Maximum size of the delay, changing this will reset the
    // internal buffer creating glitches. If smooth changes of size
    // have to be made, just change size instead, ensuring it is
    // below the maximum size.
    int max_ = 1;

    // Actual size of the delay. This is a float so we support smooth
    // choruses etc.
    float size_ = 1.0f;

    // How to interpolate fractional samples.
    Interpolation interpolation_ = LAGRANGE;
  };

  Delay();

  // Initialize with the given parameters
  absl::Status Init(const Parameters& p);

  // Fast update to parameters that don't require full reinitialization
  absl::Status FastUpdate(const Parameters& p);

  // Retrieves a sample at position size and updates the state.
  float Render(float xn);

  // Alternatively, you can read a sample at the current position,
  // before updating it. Reading it after updating it returns the
  // latest inserted value and is not what you want.
  float Read() const;

  // Reads a sample at a different offset, idx must be <= size. Linear
  // interpolation is used for floating values.
  float ReadAt(float idx) const;

  // Alternatively, you can manually write a new sample in the delay.
  void Update(float xn);

  // Size of the delay, can be set directly or via time with the
  // sample rate.
  float size() const;

  // Empty the delay, mainly used when changing position in DAW.
  void Reset();

 private:
  // Linear interpolation implementation.
  float ReadAtLinear(float at) const;

  // Lagrange interpolation implementation.
  float ReadAtLagrange(float at) const;

  // Initializes delay buffer from parameters.
  void InitFromParameters();

  Parameters params_;
  std::vector<float> buffer_;
  int idx_ = 0;
};

inline bool operator!=(const Delay::Parameters& lhs,
                       const Delay::Parameters& rhs) {
  return std::tie(lhs.max_, lhs.size_, lhs.interpolation_) !=
         std::tie(rhs.max_, rhs.size_, rhs.interpolation_);
}

}  // namespace dsp
}  // namespace soir
