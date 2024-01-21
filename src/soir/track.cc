#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "common.hh"
#include "track.hh"

namespace maethstro {
namespace soir {

Track::Track() {}

absl::Status Track::Init(const proto::Track& config) {
  channel_ = config.channel();

  volume_ = config.has_volume() ? config.volume() : kTrackDefaultVolume;
  pan_ = config.has_pan() ? config.pan() : kTrackDefaultPan;
  muted_ = config.has_muted() ? config.muted() : kTrackDefaultMuted;

  switch (config.instrument()) {

    case proto::Track::TRACK_MONO_SAMPLER: {
      instrument_ = proto::Track::TRACK_MONO_SAMPLER;
      sampler_ = std::make_unique<MonoSampler>();
      auto status = sampler_->Init(config);
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

bool Track::CanFastUpdate(const proto::Track& config) {
  std::scoped_lock<std::mutex> lock(mutex_);

  if (config.instrument() != instrument_) {
    return false;
  }

  return true;
}

void Track::FastUpdate(const proto::Track& config) {
  std::scoped_lock<std::mutex> lock(mutex_);

  volume_ = config.has_volume() ? config.volume() : kTrackDefaultVolume;
  pan_ = config.has_pan() ? config.pan() : kTrackDefaultPan;
  muted_ = config.has_muted() ? config.muted() : kTrackDefaultMuted;
}

proto::Track::Instrument Track::GetInstrument() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return instrument_;
}

int Track::GetChannel() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return channel_;
}

int Track::GetVolume() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return volume_;
}

int Track::GetPan() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return pan_;
}

bool Track::IsMuted() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return muted_;
}

void Track::HandleMidiEvent(const libremidi::message& event) {
  std::scoped_lock<std::mutex> lock(mutex_);

  auto type = event.get_message_type();

  if (type == libremidi::message_type::CONTROL_CHANGE) {
    const int control = static_cast<int>(event.bytes[1]);

    switch (control) {
      case kMidiControlMuteTrack:
        if (event.bytes[2] != 0) {
          muted_ = !muted_;
        }
        break;

      case kMidiControlVolume:
        volume_ = event.bytes[2];
        break;

      case kMidiControlPan:
        pan_ = event.bytes[2];
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

  if (!muted_) {
    sampler_->Render(events, buffer);

    // DSP per-track.
    buffer.ApplyGain(static_cast<float>(volume_) / 127.0f);
    buffer.ApplyPan(static_cast<float>(pan_) / 127.0f);
  }
}

}  // namespace soir
}  // namespace maethstro
