#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "core/dsp/track.hh"
#include "utils/misc.hh"

namespace neon {
namespace dsp {

Track::Track() {}

absl::Status Track::Init(const TrackSettings& settings,
                         SampleManager* sample_manager) {
  settings_ = settings;

  switch (settings_.instrument_) {

    case TRACK_SAMPLER: {
      settings_.instrument_ = TRACK_SAMPLER;
      sampler_ = std::make_unique<Sampler>();
      auto status = sampler_->Init(sample_manager);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to init sampler: " << status.message();
        return status;
      }
    } break;

    case TRACK_MIDI_EXT: {
      settings_.instrument_ = TRACK_MIDI_EXT;
      midi_ext_ = std::make_unique<MidiExt>();
      auto status = midi_ext_->Init(settings_.extra_);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to init midi ext: " << status.message();
        return status;
      }
      break;
    }

    default:
      return absl::InvalidArgumentError("Unknown instrument");
  }

  return absl::OkStatus();
}

bool Track::CanFastUpdate(const TrackSettings& settings) {
  std::scoped_lock<std::mutex> lock(mutex_);

  if (settings.instrument_ != settings_.instrument_) {
    return false;
  }

  return true;
}

void Track::FastUpdate(const TrackSettings& settings) {
  std::scoped_lock<std::mutex> lock(mutex_);

  settings_ = settings;

  switch (settings_.instrument_) {
    case TRACK_MIDI_EXT:
      auto status = midi_ext_->Init(settings_.extra_);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to update midi ext: " << status.message();
      }
      break;

    defaults:
      LOG(WARNING) << "Unable to fast update due to unknown instrument";
      break;
  }
}

TrackSettings Track::GetSettings() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_;
}

int Track::GetChannel() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_.channel_;
}

void Track::ProcessMidiEvents(const std::list<MidiEventAt>& events_at) {
  for (const auto& event_at : events_at) {
    auto event = event_at.Msg();
    auto type = event.get_message_type();

    if (type == libremidi::message_type::CONTROL_CHANGE) {
      const int control = static_cast<int>(event.bytes[1]);

      switch (control) {
        case kMidiControlMuteTrack:
          if (event.bytes[2] != 0) {
            settings_.muted_ = !settings_.muted_;
          }
          break;

        case kMidiControlVolume:
          settings_.volume_ = event.bytes[2];
          break;

        case kMidiControlPan:
          settings_.pan_ = event.bytes[2];
          break;

        default:
          break;
      }
    }
  }
}

void Track::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                   AudioBuffer& buffer) {
  midi_stack_.AddEvents(events);

  AudioBuffer track_buffer(buffer.Size());

  switch (settings_.instrument_) {
    case TRACK_SAMPLER:
      sampler_->Render(tick, events, track_buffer);
      break;

    case TRACK_MIDI_EXT:
      midi_ext_->Render(tick, events, track_buffer);
      break;

    defaults:
      LOG(WARNING) << "Unknown instrument";
      break;
  }

  auto ilch = track_buffer.GetChannel(kLeftChannel);
  auto irch = track_buffer.GetChannel(kRightChannel);
  auto olch = buffer.GetChannel(kLeftChannel);
  auto orch = buffer.GetChannel(kRightChannel);

  {
    std::scoped_lock<std::mutex> lock(mutex_);

    for (int i = 0; i < track_buffer.Size(); ++i) {
      std::list<MidiEventAt> events;
      midi_stack_.EventsAtTick(tick + i, events);
      ProcessMidiEvents(events);
      if (!settings_.muted_) {
        auto lgain = settings_.volume_ / 127.0f;
        auto rgain = settings_.volume_ / 127.0f;
        auto pan = settings_.pan_ / 127.0f;

        if (pan > 0.5f) {
          lgain *= (1.0f - pan) * 2.0f;
        } else {
          rgain *= pan * 2.0f;
        }

        olch[i] = ilch[i] * lgain;
        orch[i] = irch[i] * rgain;
      }
    }
  }
}

}  // namespace dsp
}  // namespace neon
