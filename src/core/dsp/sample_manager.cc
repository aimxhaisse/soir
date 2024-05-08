#include "core/dsp/sample_manager.hh"

namespace neon {
namespace dsp {

absl::Status SampleManager::Init(const utils::Config& config) {
  directory_ = config.Get<std::string>("neon.dsp.sample_directory");
  return absl::OkStatus();
}

absl::Status SampleManager::LoadPack(const std::string& name) {
  auto config_path = directory_ + "/" + name + "pack.yaml";

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (packs_.find(name) != packs_.end()) {
      return absl::OkStatus();
    }
  }

  SamplePack pack;

  auto status = pack.Init(config_path);
  if (!status.ok()) {
    return status;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  packs_[name] = std::move(pack);

  return absl::OkStatus();
}

SamplePack* SampleManager::GetPack(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  return &packs_[name];
}

}  // namespace dsp
}  // namespace neon
