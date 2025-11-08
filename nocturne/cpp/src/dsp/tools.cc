#include "dsp/tools.hh"
#include "dsp/dsp.hh"

#include <algorithm>

namespace soir {
namespace dsp {

float ClipAudioFrequency(float freq) {
  const float nyquist = (kSampleRate - 1.0f) / 2.0f;
  return std::max(1.0f, std::min(freq, nyquist));
}

float Unipolar(float bipolar) {
  return (bipolar + 1.0f) / 2.0f;
}

float Bipolar(float unipolar) {
  return (unipolar - 0.5f) * 2.0f;
}

}  // namespace dsp
}  // namespace soir
