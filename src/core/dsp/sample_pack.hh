#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <mutex>

#include "utils/config.hh"

namespace neon {
namespace dsp {

struct Sample {
  std::string path_;
  std::string name_;
  int midi_note_;
  std::vector<float> buffer_;
};

class SamplePack {
 public:
  absl::Status Init(const std::string& pack_config);

  // Do not provide a way to remove samples from a pack as it would be
  // unsafe in today's approach: the sample can be in-use in multiple
  // tracks and it's easier if we don't have to come up with
  // complexity here.

  Sample* GetSample(const std::string& name);
  Sample* GetSample(int midi_note);
  std::vector<std::string> GetSampleNames() const;

 private:
  std::map<std::string, Sample> samples_;
  std::map<int, Sample*> midi_notes_;
};

}  // namespace dsp
}  // namespace neon
