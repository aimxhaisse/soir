#pragma once

#include <absl/status/status.h>
#include <string>

namespace maethstro {
namespace soir {

class SampleConsumer {
 public:
  virtual absl::Status PushSamples(const std::string& data) = 0;
};

}  // namespace soir
}  // namespace maethstro
