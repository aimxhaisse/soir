#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <filesystem>

#include "core/dsp/mono_sampler.hh"
#include "utils/misc.hh"

namespace neon {
namespace dsp {

absl::Status MonoSampler::Init(SampleManager* sample_manager) {
  sample_manager_ = sample_manager;

  return absl::OkStatus();
}

void MonoSampler::SetSamplePack(const std::string& name) {
  sample_pack_ = sample_manager_->GetPack(name);
}

void MonoSampler::PlaySample(Sample* sample) {
  auto ps = std::make_unique<PlayingSample>();

  ps->pos_ = 0;
  ps->sample_ = sample;

  playing_[sample].push_back(std::move(ps));
}

void MonoSampler::StopSample(Sample* sample) {
  auto it = playing_.find(sample);

  if (it == playing_.end()) {
    return;
  }

  if (!it->second.empty()) {
    it->second.pop_back();
  }
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
        if (sample_pack_ == nullptr) {
          continue;
        }
        auto sample = sample_pack_->GetSample(msg.bytes[1]);
        if (sample == nullptr) {
          continue;
        }
        PlaySample(sample);
        break;
      }

      case libremidi::message_type::NOTE_OFF: {
        if (sample_pack_ == nullptr) {
          continue;
        }
        auto sample = sample_pack_->GetSample(msg.bytes[1]);
        if (sample == nullptr) {
          continue;
        }
        StopSample(sample);
        break;
      }

      default:
        continue;
    };
  }

  // DSP.
  float* left_chan = buffer.GetChannel(kLeftChannel);
  float* right_chan = buffer.GetChannel(kRightChannel);

  std::set<PlayingSample*> remove;

  for (int i = 0; i < buffer.Size(); ++i) {
    float left = left_chan[i];
    float right = right_chan[i];

    for (auto& [sample, list] : playing_) {
      for (auto& ps : list) {
        left += ps->sample_->buffer_[ps->pos_];
        right += ps->sample_->buffer_[ps->pos_];

        ps->pos_ += 1;
        if (ps->pos_ >= ps->sample_->buffer_.size()) {
          remove.insert(ps.get());
        }
      }
    }

    left_chan[i] = left;
    right_chan[i] = right;
  }

  for (auto ps : remove) {
    playing_[ps->sample_].remove_if(
        [ps](const std::unique_ptr<PlayingSample>& p) {
          return p.get() == ps;
        });
  }
}

}  // namespace dsp
}  // namespace neon
