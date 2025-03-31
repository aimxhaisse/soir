#pragma once

#include <array>

#include "core/dsp/lpf.hh"
#include "core/dsp/modulated_delay.hh"
#include "core/fast_random.hh"

namespace soir {
namespace dsp {

// Reverb engine for late reverbetations.
//
// A lot of this comes from notes extracted from Jean-Pierre Jot's
// work.
class LateReverb {
 public:
  enum MatrixFlavor {
    M4x4 = 0,   // 4x4 matrix, small quality, fast processing,
    M16x16 = 1  // 16x16 householder matrix, high quality, slow processing.
  };

  // Similar to the early reverberations, we try to keep this
  // structure as simple as possible, not for computations but to be
  // able to re-use it. Former implementations were describing all
  // parameters, but this ended up being a mess as the logic behind
  // was spread in multiple files.
  struct Parameters {
    float time_ = 0.0f;

    // Here we configure the LPF coefficients, this value can be
    // tweaked until it sounds good. This corresponds somehow to an
    // absorbtion coefficient, similar to air. The higher, the more
    // absorption. 0.2818f corresponds to air?
    float absorbency_ = 0.2818f;
  };

  LateReverb(MatrixFlavor flavor);

  void Init(const Parameters& p);
  void UpdateParameters(const Parameters& p);
  void Reset();

  std::pair<float, float> Process(float left, float right);

 private:
  void MakeMatrix();

  // This can't be changed dynamically without breaking the signal, so
  // it is assigned at construction, once.
  const MatrixFlavor flavor_ = M4x4;
  const int size_ = 4;
  const int matrixSize_ = 4 * 4;

  Parameters params_;

  // Used to initialize the mod phases at somewhat random positions.
  FastRandom random_;

  // We use the same parameters for both channels, the actual
  // modulation seed on both sides is randomly set so we get some
  // stereo-ness.
  std::vector<ModulatedDelay::Parameters> lineParams_;
  std::vector<ModulatedDelay> lLines_;
  std::vector<ModulatedDelay> rLines_;
  std::vector<float> matrix_;

  // Each line contains an LPF to absorb a bit the energy, parameters
  // are common to both channels.
  std::vector<LPF1P::Parameters> LPFParams_;
  std::vector<LPF1P> lLPFs_;
  std::vector<LPF1P> rLPFs_;

  // Those are temporary arrays to store transient state in the
  // processing main loop of the FDN network.
  std::vector<float> lDelayedValues_;
  std::vector<float> rDelayedValues_;
};

}  // namespace dsp
}  // namespace soir
