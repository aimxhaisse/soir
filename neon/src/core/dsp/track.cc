#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "core/dsp/track.hh"
#include "utils/misc.hh"

namespace neon {
namespace dsp {

Track::Track() {}

absl::Status Track::Init(const TrackSettings& settings) {
  settings_ = settings;

  switch (settings_.instrument_) {

    case TRACK_MONO_SAMPLER: {
      settings_.instrument_ = TRACK_MONO_SAMPLER;
      sampler_ = std::make_unique<MonoSampler>();
      auto status = sampler_->Init(settings);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to init sampler: " << status.message();
        return status;
      }
    } break;

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
}

TrackSettings Track::GetSettings() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_;
}

int Track::GetChannel() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_.channel_;
}

void Track::HandleMidiEvent(const libremidi::message& event) {
  std::scoped_lock<std::mutex> lock(mutex_);

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

void Track::Render(const std::list<libremidi::message>& events,
                   AudioBuffer& buffer) {
  for (auto& event : events) {
    HandleMidiEvent(event);
  }

  if (!settings_.muted_) {
    sampler_->Render(events, buffer);

    // DSP per-track.
    buffer.ApplyGain(static_cast<float>(settings_.volume_) / 127.0f);
    buffer.ApplyPan(static_cast<float>(settings_.pan_) / 127.0f);
  }
}

}  // namespace dsp
}  // namespace neon