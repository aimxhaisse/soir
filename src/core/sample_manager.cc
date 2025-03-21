#include <absl/log/log.h>
#include <absl/strings/match.h>
#include <filesystem>

#include "core/sample_manager.hh"

namespace soir {

absl::Status SampleManager::Init(const utils::Config& config) {
  directory_ = config.Get<std::string>("soir.dsp.sample_directory");
  if (directory_.empty()) {
    LOG(WARNING) << "No sample directory specified in config";
    return absl::OkStatus();
  }

  if (!std::filesystem::exists(directory_)) {
    return absl::NotFoundError("Sample directory " + directory_ +
                               " does not exists");
  }

  for (const auto& entry : std::filesystem::directory_iterator(directory_)) {
    if (!entry.is_directory()) {
      auto candidate_pack = entry.path().filename().string();

      if (absl::EndsWith(candidate_pack, ".pack.yaml")) {
        auto pack = candidate_pack.substr(0, candidate_pack.size() - 10);
        auto status = LoadPack(pack);
        if (!status.ok()) {
          return status;
        }
      }
    }
  }

  return absl::OkStatus();
}

absl::Status SampleManager::LoadPack(const std::string& name) {
  auto config_path = directory_ + "/" + name + ".pack.yaml";

  LOG(INFO) << "Loading pack: " << name;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (packs_.find(name) != packs_.end()) {
      return absl::OkStatus();
    }
  }

  SamplePack pack;

  auto status = pack.Init(directory_, config_path);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to load pack " << name << ": " << status;
    return status;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    packs_[name] = std::move(pack);
  }

  return absl::OkStatus();
}

SamplePack* SampleManager::GetPack(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  return &packs_[name];
}

std::vector<std::string> SampleManager::GetPackNames() {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<std::string> names;
  for (const auto& [name, _] : packs_) {
    names.push_back(name);
  }

  return names;
}

}  // namespace soir
