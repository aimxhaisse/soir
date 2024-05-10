#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <absl/strings/match.h>
#include <filesystem>
#include <mutex>

#include "core/dsp/dsp.hh"
#include "core/dsp/sample_pack.hh"
#include "utils/config.hh"

namespace neon {
namespace dsp {

absl::Status SamplePack::Init(const std::string& pack_config) {
  auto config_or = utils::Config::LoadFromPath(pack_config);
  if (!config_or.ok()) {
    return config_or.status();
  }
  auto config = std::move(*config_or);

  auto sample_configs = config->GetConfigs("samples");
  if (sample_configs.empty()) {
    return absl::InvalidArgumentError("No samples found in pack");
  }

  for (auto& sample_config : sample_configs) {
    Sample s;

    s.name_ = sample_config->Get<std::string>("name");
    s.path_ = sample_config->Get<std::string>("path");

    AudioFile<float> audio_file;

    if (!audio_file.load(s.path_)) {
      return absl::InvalidArgumentError("Failed to load sample " + s.path_);
    }
    if (audio_file.getNumChannels() != 1) {
      return absl::InvalidArgumentError(
          "Only mono samples are supported for now");
    }
    if (audio_file.getSampleRate() != kSampleRate) {
      return absl::InvalidArgumentError(
          "Only 48000Hz sample rate is supported for now");
    }

    s.buffer_ = audio_file.samples[0];

    samples_[s.name_] = std::move(s);

    LOG(INFO) << "Loaded sample " << s.name_;
  }

  LOG(INFO) << "Loaded " << samples_.size() << " samples";

  return absl::OkStatus();
}

Sample* SamplePack::GetSample(const std::string& pattern) {
  auto it = samples_.find(pattern);
  if (it == samples_.end()) {
    for (auto& [name, s] : samples_) {
      if (absl::StrContains(pattern, name)) {
        return &s;
      }
    }

    return nullptr;
  }

  return &it->second;
}

std::vector<std::string> SamplePack::GetSampleNames() const {
  std::vector<std::string> names;
  for (const auto& [name, _] : samples_) {
    names.push_back(name);
  }
  return names;
}

}  // namespace dsp
}  // namespace neon
