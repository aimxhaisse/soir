#include <algorithm>
#include <cstdint>

#include <AudioFile.h>
#include <absl/log/log.h>
#include <rapidjson/document.h>
#include <filesystem>

#include "core/dsp/sampler.hh"
#include "core/dsp/tools.hh"
#include "utils/misc.hh"

namespace soir {
namespace dsp {

absl::Status Sampler::Init(SampleManager* sample_manager) {
  sample_manager_ = sample_manager;

  return absl::OkStatus();
}

void Sampler::PlaySample(Sample* sample, float start, float end, float pan,
                         float a, float d, float s, float r) {
  if (sample == nullptr) {
    return;
  }

  int duration = static_cast<int>(sample->DurationSamples());
  int sstart = std::max(
      0,
      std::min(static_cast<int>(sample->DurationSamples() * start), duration));
  int ssend = std::max(
      0, std::min(static_cast<int>(sample->DurationSamples() * end), duration));

  const float durationMs = sample->DurationMs(std::abs(ssend - sstart));
  if (durationMs <= kSampleMinimalDurationMs) {
    return;
  }

  auto ps = std::make_unique<PlayingSample>();

  ps->start_ = sstart;
  ps->end_ = ssend;
  ps->pos_ = sstart;
  ps->sample_ = sample;
  ps->inc_ = (sstart < ssend) ? 1 : -1;
  ps->pan_ = pan;

  // This is for the envelope to prevent glitches.

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

  // This is for the envelope controlled by the user.

  status = ps->env_.Init(a, d, s, r);
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

void Sampler::HandleSysex(const proto::MidiSysexInstruction& sysex) {
  rapidjson::Document params;

  params.Parse(sysex.json_payload().c_str());

  auto pack = params["pack"].GetString();
  auto name = params["name"].GetString();

  // Offsets

  float start = 0.0f;
  float end = 1.0f;

  if (params.HasMember("start")) {
    start = params["start"].GetDouble();
  }
  if (params.HasMember("end")) {
    end = params["end"].GetDouble();
  }

  // Pan

  float pan = 0.0f;

  if (params.HasMember("pan")) {
    pan = params["pan"].GetDouble();
  }

  // Envelope

  float a = 0.0f;
  float d = 0.0f;
  float s = 1.0f;
  float r = 0.0f;

  if (params.HasMember("attack")) {
    a = params["attack"].GetDouble() * 1000.0f;
  }
  if (params.HasMember("decay")) {
    d = params["decay"].GetDouble() * 1000.0f;
  }
  if (params.HasMember("sustain")) {
    s = params["sustain"].GetDouble();
  }
  if (params.HasMember("release")) {
    r = params["release"].GetDouble() * 1000.0f;
  }

  switch (sysex.type()) {
    case proto::MidiSysexInstruction::SAMPLER_PLAY: {
      PlaySample(GetSample(pack, name), start, end, pan, a, d, s, r);
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
}

void Sampler::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                     AudioBuffer& buffer) {
  midi_stack_.AddEvents(events);

  float* left_chan = buffer.GetChannel(kLeftChannel);
  float* right_chan = buffer.GetChannel(kRightChannel);

  std::set<PlayingSample*> remove;

  for (int i = 0; i < buffer.Size(); ++i) {
    ProcessMidiEvents(tick + i);

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
        const float env = wrapper_env * user_env;

        left += ps->sample_->lb_[ps->pos_] * env * LeftPan(ps->pan_);
        right += ps->sample_->rb_[ps->pos_] * env * RightPan(ps->pan_);

        ps->pos_ += ps->inc_;
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

}  // namespace dsp
}  // namespace soir
