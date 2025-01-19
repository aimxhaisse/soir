#include "core/dsp/sample.hh"
#include "core/dsp/dsp.hh"

namespace soir {
namespace dsp {

float Sample::DurationMs(std::size_t samples) const {
  return static_cast<float>(samples) / kSampleRate * 1000.0f;
}

float Sample::DurationMs() const {
  return DurationMs(DurationSamples());
}

std::size_t Sample::DurationSamples() const {
  return lb_.size();
}

}  // namespace dsp
}  // namespace soir
