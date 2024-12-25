#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <mutex>

#include "utils/config.hh"

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

class SamplePack {
 public:
  absl::Status Init(const std::string& dir, const std::string& pack_config);

  // Do not provide a way to remove samples from a pack as it would be
  // unsafe in today's approach: the sample can be in-use in multiple
  // tracks and it's easier if we don't have to come up with
  // complexity here.

  Sample* GetSample(const std::string& name);
  std::vector<std::string> GetSampleNames() const;

 private:
  std::map<std::string, Sample> samples_;
};

}  // namespace dsp
}  // namespace soir
