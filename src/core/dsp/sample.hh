#pragma once

#include <string>
#include <vector>

namespace soir {
namespace dsp {

struct Sample {
  std::string path_;
  std::string name_;
  std::vector<float> lb_;
  std::vector<float> rb_;

  float DurationMs() const;
  float DurationMs(std::size_t samples) const;
  std::size_t DurationSamples() const;
};

}  // namespace dsp
}  // namespace soir
