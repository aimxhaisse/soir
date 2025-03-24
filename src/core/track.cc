#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "core/tools.hh"
#include "core/track.hh"
#include "utils/misc.hh"

namespace soir {

Track::Track() {}

absl::Status Track::Init(const Settings& settings,
                         SampleManager* sample_manager, Controls* controls) {
  settings_ = settings;
  controls_ = controls;
  sample_manager_ = sample_manager;

  switch (settings_.instrument_) {
    case inst::Type::SAMPLER: {
      settings_.instrument_ = inst::Type::SAMPLER;
      inst_ = std::make_unique<inst::Sampler>();
    } break;

    case inst::Type::MIDI_EXT: {
      settings_.instrument_ = inst::Type::MIDI_EXT;
      inst_ = std::make_unique<inst::MidiExt>();
      break;
    }

    default:
      return absl::InvalidArgumentError("Unknown instrument");
  }

  auto status = inst_->Init(settings_.extra_, sample_manager_, controls_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init instrument: " << status.message();
    return status;
  }
  status = inst_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start instrument: " << status.message();
    return status;
  }

  fx_stack_ = std::make_unique<fx::FxStack>(controls_);

  status = fx_stack_->Init(settings_.fxs_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init fx stack: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Track::Stop() {
  return inst_->Stop();
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

  auto status = inst_->Init(settings_.extra_, sample_manager_, controls_);
  if (!status.ok()) {
    LOG(WARNING) << "Failed to fast-update instrument: " << status.message();
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

  inst_->Render(tick, events, track_buffer);

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

}  // namespace soir
