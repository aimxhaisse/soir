#pragma once

#include <absl/status/status.h>
#include <list>
#include <map>
#include <mutex>

#include "utils/config.hh"

namespace neon {
namespace dsp {

struct Sample {
  std::string directory_;
  std::string path_;
  std::string name_;
  std::vector<float> buffer_;
};

class SampleManager {
 public:
  absl::Status Init(const utils::Config& config);
  absl::Status LoadFromDirectory(const std::string& directory);
  Sample GetSample(const std::string& name);

 private:
  std::mutex mutex_;
  std::string cache_directory_;
  std::map<std::string, Sample> samples_;
};

}  // namespace dsp
}  // namespace neon
