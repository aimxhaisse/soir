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
  Sample* GetSample(const std::string& name);
  Sample* GetSample(int midi_note);

 private:
  std::map<std::string, Sample> samples_;
  std::map<int, Sample*> midi_notes_;
};

}  // namespace dsp
}  // namespace neon
