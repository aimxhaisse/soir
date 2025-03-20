#include "core/engine/sample.hh"
#include "core/engine/dsp.hh"

namespace soir {
namespace engine {

float Sample::DurationMs(std::size_t samples) const {
  return static_cast<float>(samples) / kSampleRate * 1000.0f;
}

float Sample::DurationMs() const {
  return DurationMs(DurationSamples());
}

std::size_t Sample::DurationSamples() const {
  return lb_.size();
}

}  // namespace engine
}  // namespace soir
