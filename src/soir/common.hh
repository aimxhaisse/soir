#pragma once

#include <absl/status/status.h>
#include <vector>

namespace maethstro {
namespace soir {

using Samples = std::vector<float>;

class SampleConsumer {
 public:
  virtual absl::Status PushSamples(const Samples& data) = 0;
};

}  // namespace soir
}  // namespace maethstro
