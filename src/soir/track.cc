#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "common.hh"
#include "track.hh"

namespace maethstro {
namespace soir {

Track::Track() {}

absl::Status Track::Init(const common::Config& config) {
  channel_ = config.Get<int>("channel");

  auto instrument = config.Get<std::string>("instrument");
  if (instrument != "sampler") {
    return absl::InvalidArgumentError("Only sampler instrument is supported");
  }

  auto directory = config.Get<std::string>("sample_dir");

  std::vector<std::string> samples;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      samples.push_back(entry.path());
    }
  }
  std::sort(samples.begin(), samples.end());

  int midi_note = 0;
  for (auto& sample : samples) {
    AudioFile<float> audio_file;

    if (!audio_file.load(sample)) {
      return absl::InvalidArgumentError("Failed to load sample " + sample);
    }
    if (audio_file.getNumChannels() != 1) {
      return absl::InvalidArgumentError(
          "Only mono samples are supported for now");
    }
    if (audio_file.getSampleRate() != kSampleRate) {
      return absl::InvalidArgumentError(
          "Only 48000Hz sample rate is supported for now");
    }

    auto mono_sampler = std::make_unique<MonoSampler>();

    mono_sampler->buffer_ = audio_file.samples[0];
    mono_sampler->is_playing_ = false;
    mono_sampler->pos_ = 0;

    samples_[midi_note] = std::move(mono_sampler);

    LOG(INFO) << "Loaded sample " << sample << " for note " << midi_note;

    midi_note += 1;
  }

  LOG(INFO) << "Loaded track " << channel_ << " with " << samples_.size()
            << " samples";

  return absl::OkStatus();
}

void Track::Render(const std::list<proto::MidiEvents_Request>&,
                   AudioBuffer& buffer) {}

}  // namespace soir
}  // namespace maethstro
