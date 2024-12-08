#pragma once

#include <mutex>

#include "core/dsp/sample_pack.hh"
#include "utils/config.hh"

namespace soir {
namespace dsp {

class SampleManager {
 public:
  absl::Status Init(const utils::Config& config);
  absl::Status LoadPack(const std::string& path);

  SamplePack* GetPack(const std::string& name);
  std::vector<std::string> GetPackNames();

 private:
  std::string directory_;

  std::mutex mutex_;
  std::map<std::string, SamplePack> packs_;
};

}  // namespace dsp
}  // namespace soir
