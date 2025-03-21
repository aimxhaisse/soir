#include <algorithm>
#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <filesystem>

#include "core/sampler.hh"
#include "core/tools.hh"
#include "utils/misc.hh"

namespace soir {

absl::Status Sampler::Init(SampleManager* sample_manager, Controls* controls) {
  sample_manager_ = sample_manager;
  controls_ = controls;

  return absl::OkStatus();
}

void Sampler::PlaySample(Sample* sample, const PlaySampleParameters& p) {
  if (sample == nullptr) {
    return;
  }

  // We don't scale this to the rate, this is the window in which we look
  // for samples.
  int range = static_cast<int>(sample->DurationSamples());
  int sstart = std::max(0, std::min(static_cast<int>(range * p.start_), range));
  int ssend = std::max(0, std::min(static_cast<int>(range * p.end_), range));

  // Prevent glitches if the sample to play is too small. We might
  // need to find a better approach here, devices like elektron
  // machines can play in loop very short samples without glitch,
  // likely through some interpolation.
  const float durationMs =
      sample->DurationMs(std::abs(ssend - sstart)) * p.rate_;
  if (durationMs <= kSampleMinimalDurationMs) {
    return;
  }

  auto ps = std::make_unique<PlayingSample>();

  ps->start_ = sstart;
  ps->end_ = ssend;
  ps->pos_ = sstart;
  ps->sample_ = sample;
  ps->inc_ = (sstart < ssend) ? 1 : -1;
  ps->pan_ = p.pan_;
  ps->rate_ = p.rate_;
  ps->amp_ = p.amp_;

  // This is for the envelope to prevent glitches.

  const float attackMs = kSampleMinimalSmoothingMs;
  const float releaseMs = kSampleMinimalSmoothingMs;
  const float decayMs = 0.0f;
  const float level = 1.0f;

  absl::Status status = ps->wrapper_.Init(attackMs, decayMs, level, releaseMs);
  if (status != absl::OkStatus()) {
    LOG(WARNING) << "Failed to initialize envelope in play sample: " << status;
  }

  ps->wrapper_.NoteOn();

  // This is for the envelope controlled by the user.

  status = ps->env_.Init(p.attack_, p.decay_, p.release_, p.level_);
  if (status != absl::OkStatus()) {
    LOG(WARNING) << "Failed to initialize envelope in play sample: " << status;
  }
  ps->env_.NoteOn();

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

void Sampler::PlaySampleParameters::FromJson(Controls* controls,
                                             const rapidjson::Value& json,
                                             PlaySampleParameters* p) {
  // Offsets
  if (json.HasMember("start")) {
    p->start_ = json["start"].GetDouble();
  }
  if (json.HasMember("end")) {
    p->end_ = json["end"].GetDouble();
  }

  // Pan
  if (json.HasMember("pan")) {
    if (json["pan"].IsString()) {
      const std::string pan = json["pan"].GetString();
      p->pan_.SetControl(controls, pan);
    } else {
      p->pan_.SetConstant(json["pan"].GetDouble());
    }
  }

  // Playback rate
  if (json.HasMember("rate")) {
    p->rate_ = json["rate"].GetDouble();

    // This is a trick, if the rate is negative, we want to play the
    // sample backward. As we already handle inverted start/end to do
    // so, we re-use the same mecanism here to not have to fiddle too
    // much with the rendering which is already complex.
    if (p->rate_ < 0.0f) {
      float swap = p->start_;

      p->start_ = p->end_;
      p->end_ = swap;

      p->rate_ = -p->rate_;
    }
  }

  // Envelope
  if (json.HasMember("attack")) {
    p->attack_ = json["attack"].GetDouble();
  }
  if (json.HasMember("decay")) {
    p->decay_ = json["decay"].GetDouble();
  }
  if (json.HasMember("level")) {
    p->level_ = json["level"].GetDouble();
  }
  if (json.HasMember("release")) {
    p->release_ = json["release"].GetDouble();
  }

  // Amplitude
  if (json.HasMember("amp")) {
    p->amp_ = json["amp"].GetDouble();
  }
}

void Sampler::HandleSysex(const proto::MidiSysexInstruction& sysex) {
  rapidjson::Document params;

  params.Parse(sysex.json_payload().c_str());

  auto pack = params["pack"].GetString();
  auto name = params["name"].GetString();

  switch (sysex.type()) {
    case proto::MidiSysexInstruction::SAMPLER_PLAY: {
      PlaySampleParameters p;
      PlaySampleParameters::FromJson(controls_, params, &p);
      PlaySample(GetSample(pack, name), p);
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

void Sampler::ProcessMidiEvents(SampleTick tick) {
  std::list<MidiEventAt> events_at;

  midi_stack_.EventsAtTick(tick, events_at);

  // Process MIDI events.
  //
  // For now we don't have timing information in MIDI events so we
  // don't split this code in a dedicated function as the two logics
  // (midi/DSP) will be interleaved at some point.
  for (auto event_at : events_at) {
    auto msg = event_at.Msg();
    auto type = msg.get_message_type();

    switch (type) {
      case libremidi::message_type::SYSTEM_EXCLUSIVE: {
        proto::MidiSysexInstruction sysex;
        if (!sysex.ParseFromArray(msg.bytes.data() + 1, msg.bytes.size() - 1)) {
          LOG(WARNING) << "Failed to parse sysex message in sampler";
          break;
        }

        HandleSysex(sysex);

        break;
      }

      default:
        continue;
    };
  }
}

float Sampler::Interpolate(const std::vector<float>& v, float pos) {
  // Here the goal is to find an interpolated value between two
  // samples. We are using a linear interpolation for now but we could
  // use a more complex interpolation method in the future.

  float v0 = v[static_cast<int>(pos) % v.size()];
  float v1 = v[(static_cast<int>(pos) + 1) % v.size()];

  float w1 = pos - static_cast<int>(pos);
  float w0 = 1.0f - w1;

  return v0 * w0 + v1 * w1;
}

void Sampler::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                     AudioBuffer& buffer) {
  midi_stack_.AddEvents(events);

  float* left_chan = buffer.GetChannel(kLeftChannel);
  float* right_chan = buffer.GetChannel(kRightChannel);

  std::set<PlayingSample*> remove;

  for (int i = 0; i < buffer.Size(); ++i) {
    const SampleTick current_tick = tick + i;

    ProcessMidiEvents(current_tick);

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
        if ((ps->inc_ > 0 &&
             ps->pos_ + kSampleMinimalSmoothingSamples >= ps->end_) ||
            (ps->inc_ < 0 &&
             ps->pos_ - kSampleMinimalSmoothingSamples <= ps->end_)) {
          ps->wrapper_.NoteOff();
        }

        const float wrapper_env = ps->wrapper_.GetNextEnvelope();
        const float user_env = ps->env_.GetNextEnvelope();
        const float env = wrapper_env * user_env * ps->amp_;
        const float pan = ps->pan_.GetValue(current_tick);

        left += Interpolate(ps->sample_->lb_, ps->pos_) * env * LeftPan(pan);
        right += Interpolate(ps->sample_->rb_, ps->pos_) * env * RightPan(pan);

        // Update the position of the sample taking into account the rate
        // of playback.
        ps->pos_ += static_cast<float>(ps->inc_) * ps->rate_;

        if (env == 0.0f || (ps->inc_ > 0 && (ps->pos_ >= ps->end_)) ||
            (ps->inc_ < 0 && (ps->pos_ <= ps->end_))) {
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

}  // namespace soir
