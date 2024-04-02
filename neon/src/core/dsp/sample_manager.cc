#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <filesystem>
#include <mutex>

#include "core/dsp/dsp.hh"
#include "core/dsp/sample_manager.hh"

namespace neon {
namespace dsp {

absl::Status SampleManager::Init(const utils::Config& config) {
  cache_directory_ = config.Get<std::string>("neon.dsp.sample.cache_directory");

  std::filesystem::create_directories(cache_directory_);

  LOG(INFO) << "Sample cache directory: " << cache_directory_;

  return absl::OkStatus();
}

absl::Status SampleManager::LoadFromDirectory(const std::string& directory) {
  std::vector<std::pair<std::string, std::string>> samples;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      samples.push_back(std::make_pair<std::string, std::string>(
          entry.path(), std::filesystem::path(entry).filename()));
    }
  }

  for (auto& entry : samples) {
    const auto& sample_path = entry.first;
    const auto& sample_name = entry.second;
    AudioFile<float> audio_file;

    if (!audio_file.load(sample_path)) {
      return absl::InvalidArgumentError("Failed to load sample " + sample_path);
    }
    if (audio_file.getNumChannels() != 1) {
      return absl::InvalidArgumentError(
          "Only mono samples are supported for now");
    }
    if (audio_file.getSampleRate() != kSampleRate) {
      return absl::InvalidArgumentError(
          "Only 48000Hz sample rate is supported for now");
    }

    Sample sample;
    sample.directory_ = directory;
    sample.name_ = sample_name;
    sample.path_ = sample_path;
    sample.buffer_ = audio_file.samples[0];

    {
      std::scoped_lock<std::mutex> lock(mutex_);

      samples_[sample_name] = std::move(sample);
    }

    LOG(INFO) << "Loaded sample " << sample_name << " from " << sample_path;
  }

  LOG(INFO) << "Loaded " << samples.size() << " samples";

  return absl::OkStatus();
}

Sample SampleManager::GetSample(const std::string& name) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return samples_.at(name);
}

}  // namespace dsp
}  // namespace neon
