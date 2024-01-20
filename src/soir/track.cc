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
  sampler_ = std::make_unique<MonoSampler>();

  auto status = sampler_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init sampler: " << status.message();
    return status;
  }

  return absl::OkStatus();
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
