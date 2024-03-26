#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <filesystem>

#include "mono_sampler.hh"
#include "utils/misc.hh"

namespace neon {
namespace dsp {

absl::Status MonoSampler::Init(const TrackSettings& settings) {
  // TODO: add this to the parameters of the track.
  auto directory = "assets/samples/pack/foundation";

  std::vector<std::string> samples;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      samples.push_back(entry.path());
    }
  }
  std::sort(samples.begin(), samples.end());

  int midi_note = 0;
  for (auto& sample_path : samples) {
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

    auto sampler = std::make_unique<Sampler>();

    sampler->buffer_ = audio_file.samples[0];
    sampler->is_playing_ = false;
    sampler->pos_ = 0;

    samplers_[midi_note] = std::move(sampler);

    LOG(INFO) << "Loaded sample " << sample_path << " for note " << midi_note;

    midi_note += 1;
  }

  LOG(INFO) << "Loaded track with " << samplers_.size() << " samples";

  return absl::OkStatus();
}

void MonoSampler::Render(const std::list<libremidi::message>& messages,
                         AudioBuffer& buffer) {
  // Process MIDI events.
  //
  // For now we don't have timing information in MIDI events so we
  // don't split this code in a dedicated function as the two logics
  // (midi/DSP) will be interleaved at some point.
  for (auto msg : messages) {
    auto type = msg.get_message_type();

    switch (type) {
      case libremidi::message_type::NOTE_ON: {
        auto it = samplers_.find(msg.bytes[1]);
        if (it != samplers_.end()) {
          auto& sampler = it->second;
          sampler->NoteOn();
          playing_.insert(sampler.get());
        }
        break;
      }

      case libremidi::message_type::NOTE_OFF: {
        auto it = samplers_.find(msg.bytes[1]);
        if (it != samplers_.end()) {
          auto& sampler = it->second;
          sampler->NoteOff();
          playing_.erase(sampler.get());
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

  std::set<Sampler*> to_remove;

  for (int sample = 0; sample < buffer.Size(); ++sample) {
    for (auto& sampler : playing_) {
      float left = left_chan[sample];
      float right = right_chan[sample];

      left += sampler->buffer_[sampler->pos_];
      right += sampler->buffer_[sampler->pos_];

      left_chan[sample] = left;
      right_chan[sample] = right;

      sampler->pos_ += 1;
      if (sampler->pos_ >= sampler->buffer_.size()) {
        sampler->NoteOff();
        to_remove.insert(sampler);
      }
    }
  }

  for (auto remove : to_remove) {
    playing_.erase(remove);
  }
}

void MonoSampler::Sampler::NoteOn() {
  is_playing_ = true;
  pos_ = 0;
}

void MonoSampler::Sampler::NoteOff() {
  is_playing_ = false;
  pos_ = 0;
}

}  // namespace dsp
}  // namespace neon
