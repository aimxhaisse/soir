#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "core/dsp/tools.hh"
#include "core/dsp/track.hh"
#include "utils/misc.hh"

namespace soir {
namespace dsp {

Track::Track() {}

absl::Status Track::Init(const Settings& settings,
                         SampleManager* sample_manager, Controls* controls) {
  settings_ = settings;
  controls_ = controls;

  switch (settings_.instrument_) {

    case TRACK_SAMPLER: {
      settings_.instrument_ = TRACK_SAMPLER;
      sampler_ = std::make_unique<Sampler>();
      auto status = sampler_->Init(sample_manager, controls);
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

      status = midi_ext_->Start();
      if (!status.ok()) {
        LOG(ERROR) << "Failed to start midi ext: " << status.message();
        return status;
      }

      break;
    }

    default:
      return absl::InvalidArgumentError("Unknown instrument");
  }

  fx_stack_ = std::make_unique<FxStack>(controls_);

  auto status = fx_stack_->Init(settings_.fxs_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init fx stack: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Track::Stop() {
  switch (settings_.instrument_) {
    case TRACK_MIDI_EXT:
      return midi_ext_->Stop();
    default:
      return absl::OkStatus();
  }
}

bool Track::CanFastUpdate(const Settings& settings) {
  std::scoped_lock<std::mutex> lock(mutex_);

  if (settings.instrument_ != settings_.instrument_) {
    return false;
  }

  if (!fx_stack_->CanFastUpdate(settings.fxs_)) {
    return false;
  }

  return true;
}

void Track::FastUpdate(const Settings& settings) {
  std::scoped_lock<std::mutex> lock(mutex_);

  settings_ = settings;

  switch (settings_.instrument_) {
    case TRACK_MIDI_EXT: {
      auto status = midi_ext_->Init(settings_.extra_);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to update midi ext: " << status.message();
      }
    } break;

    case TRACK_SAMPLER:
      break;

    case TRACK_UNKNOWN:
    defaults:
      LOG(WARNING) << "Unable to fast update due to unknown instrument";
      break;
  }

  fx_stack_->FastUpdate(settings_.fxs_);
}

Track::Settings Track::GetSettings() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_;
}

const std::string& Track::GetTrackName() {
  std::scoped_lock<std::mutex> lock(mutex_);

  return settings_.name_;
}

void Track::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                   AudioBuffer& buffer) {
  AudioBuffer track_buffer(buffer.Size());

  switch (settings_.instrument_) {
    case TRACK_SAMPLER:
      sampler_->Render(tick, events, track_buffer);
      break;

    case TRACK_MIDI_EXT:
      midi_ext_->Render(tick, events, track_buffer);
      break;

    case TRACK_UNKNOWN:
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
      SampleTick current_tick = tick + i;

      if (!settings_.muted_) {
        const float vol = settings_.volume_.GetValue(current_tick);
        const float pan = settings_.pan_.GetValue(current_tick);

        auto lgain = vol * LeftPan(pan);
        auto rgain = vol * RightPan(pan);

        olch[i] += ilch[i] * lgain;
        orch[i] += irch[i] * rgain;
      }
    }

    fx_stack_->Render(tick, buffer);
  }
}

}  // namespace dsp
}  // namespace soir
