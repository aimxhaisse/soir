#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>
#include <libremidi/libremidi.hpp>

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

void Track::Render(const std::list<proto::MidiEvents_Request>& midi_events,
                   AudioBuffer& buffer) {
  // Process MIDI events.
  //
  // For now we don't have timing information in MIDI events so we
  // don't split this code in a dedicated function as the two logics
  // (midi/DSP) will be interleaved at some point.
  for (auto midi_event : midi_events) {
    const std::string& raw = midi_event.midi_payload();
    libremidi::message msg;
    msg.bytes = libremidi::midi_bytes(raw.begin(), raw.end());

    if (msg.get_channel() != channel_) {
      continue;
    }

    auto type = msg.get_message_type();

    switch (type) {
      case libremidi::message_type::NOTE_ON: {
        auto it = samples_.find(msg.bytes[1]);
        if (it != samples_.end()) {
          auto& sampler = it->second;
          sampler->is_playing_ = true;
          sampler->pos_ = 0;
        }
        break;
      }

      case libremidi::message_type::NOTE_OFF: {
        auto it = samples_.find(msg.bytes[1]);
        if (it != samples_.end()) {
          auto& sampler = it->second;
          sampler->is_playing_ = false;
          sampler->pos_ = 0;
        }
        break;
      }

      default:
        continue;
    };
  }

  // DSP.
  float* left_chan = buffer.GetChannel(kLeftChannel);
  float* right_chan = buffer.GetChannel(kRightChannel);

  for (int sample = 0; sample < buffer.Size(); ++sample) {
    for (auto& it : samples_) {
      auto& sampler = it.second;

      if (!sampler->is_playing_) {
        continue;
      }

      float left = left_chan[sample];
      float right = right_chan[sample];

      left += sampler->buffer_[sampler->pos_];
      right += sampler->buffer_[sampler->pos_];

      left_chan[sample] = left;
      right_chan[sample] = right;

      sampler->pos_ += 1;
      if (sampler->pos_ >= sampler->buffer_.size()) {
        sampler->is_playing_ = false;
        sampler->pos_ = 0;
      }
    }
  }
}

}  // namespace soir
}  // namespace maethstro
