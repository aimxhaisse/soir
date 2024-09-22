#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <rapidjson/document.h>
#include <filesystem>

#include "core/dsp/sampler.hh"
#include "utils/misc.hh"

namespace neon {
namespace dsp {

absl::Status Sampler::Init(SampleManager* sample_manager) {
  sample_manager_ = sample_manager;

  return absl::OkStatus();
}

void Sampler::PlaySample(Sample* sample) {
  if (sample == nullptr) {
    return;
  }

  const float durationMs = sample->DurationMs();
  if (durationMs <= kSampleMinimalDurationMs) {
    return;
  }

  auto ps = std::make_unique<PlayingSample>();

  ps->pos_ = 0;
  ps->sample_ = sample;

  const float attackMs = kSampleMinimalSmoothingMs;
  const float releaseMs = kSampleMinimalSmoothingMs;
  const float decayMs = 0.0f;
  const float sustainLevel = 1.0f;

  absl::Status status =
      ps->wrapper_.Init(attackMs, decayMs, sustainLevel, releaseMs);
  if (status != absl::OkStatus()) {
    LOG(WARNING) << "Failed to initialize envelope in play sample: " << status;
  }

  ps->wrapper_.NoteOn();

  playing_[sample].push_back(std::move(ps));
}

void Sampler::StopSample(Sample* sample) {
  if (sample == nullptr) {
    return;
  }

  auto it = playing_.find(sample);

  if (it == playing_.end()) {
    return;
  }

  if (!it->second.empty()) {
    auto ps = it->second.back().get();

    ps->wrapper_.NoteOff();
  }
}

Sample* Sampler::GetSample(const std::string& pack, const std::string& name) {
  auto sample_pack = sample_manager_->GetPack(pack);
  if (sample_pack == nullptr) {
    return nullptr;
  }

  return sample_pack->GetSample(name);
}

void Sampler::HandleSysex(const proto::MidiSysexInstruction& sysex) {
  rapidjson::Document params;

  params.Parse(sysex.json_payload().c_str());

  auto pack = params["pack"].GetString();
  auto name = params["name"].GetString();

  switch (sysex.type()) {
    case proto::MidiSysexInstruction::SAMPLER_PLAY: {
      PlaySample(GetSample(pack, name));
      break;
    }

    case proto::MidiSysexInstruction::SAMPLER_STOP: {
      StopSample(GetSample(pack, name));
      break;
    }

    default:
      LOG(WARNING) << "Unknown sysex instruction";
      break;
  }
}

void Sampler::Render(const std::list<libremidi::message>& messages,
                     AudioBuffer& buffer) {
  // Process MIDI events.
  //
  // For now we don't have timing information in MIDI events so we
  // don't split this code in a dedicated function as the two logics
  // (midi/DSP) will be interleaved at some point.
  for (auto msg : messages) {
    auto type = msg.get_message_type();

    switch (type) {
      case libremidi::message_type::SYSTEM_EXCLUSIVE: {
        proto::MidiSysexInstruction sysex;
        if (!sysex.ParseFromArray(msg.bytes.data() + 1, msg.bytes.size() - 1)) {
          LOG(WARNING) << "Failed to parse sysex message";
          break;
        }

        HandleSysex(sysex);

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
        if (ps->removing_) {
          continue;
        }

        // Trigger a note-off if we are near the very end of the
        // sample ; this is to ensure we do not glitch at the end of
        // the sample.
        if (ps->pos_ + kSampleMinimalSmoothingMs >=
            ps->sample_->DurationSamples()) {
          ps->wrapper_.NoteOff();
        }

        const float env = ps->wrapper_.GetNextEnvelope();

        left += ps->sample_->lb_[ps->pos_] * env;
        right += ps->sample_->rb_[ps->pos_] * env;

        ps->pos_ += 1;
        if (env == 0.0f || ps->pos_ >= ps->sample_->DurationSamples()) {
          ps->removing_ = true;
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
