#include "core/track.hh"

#include <absl/log/log.h>
#include <absl/status/status.h>

#include <filesystem>

#include "utils/tools.hh"
#include "vst/vst_host.hh"

namespace soir {

Track::Track() : track_buffer_(kBlockSize) {}

Track::~Track() { Stop().IgnoreError(); }

absl::Status Track::Init(const Settings& settings,
                         SampleManager* sample_manager, Controls* controls,
                         vst::VstHost* vst_host) {
  settings_ = settings;
  controls_ = controls;
  sample_manager_ = sample_manager;
  vst_host_ = vst_host;

  switch (settings_.instrument_) {
    case inst::Type::SAMPLER: {
      settings_.instrument_ = inst::Type::SAMPLER;
      inst_ = std::make_unique<inst::Sampler>();
    } break;

    case inst::Type::EXTERNAL: {
      settings_.instrument_ = inst::Type::EXTERNAL;
      inst_ = std::make_unique<inst::External>();
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

  fx_stack_ = std::make_unique<fx::FxStack>(controls_, vst_host_);

  status = fx_stack_->Init(settings_.fxs_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init fx stack: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

absl::Status Track::Start() {
  LOG(INFO) << "Starting track thread for: " << settings_.name_;

  // Initialize thread state
  stop_thread_ = false;
  has_work_ = false;
  work_done_ = true;

  // Start the processing thread
  thread_ = std::thread([this]() {
    auto status = ProcessLoop();
    if (!status.ok()) {
      LOG(ERROR) << "Track processing thread failed: " << status;
    }
  });

  return absl::OkStatus();
}

absl::Status Track::Stop() {
  LOG(INFO) << "Stopping track thread for: " << settings_.name_;

  if (thread_.joinable()) {
    {
      std::lock_guard<std::mutex> lock(work_mutex_);
      stop_thread_ = true;
      work_cv_.notify_one();
    }
    thread_.join();
  }

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

Levels Track::GetLevels() const { return level_meter_.GetLevels(); }

void Track::RenderAsync(SampleTick tick, const std::list<MidiEventAt>& events) {
  std::lock_guard<std::mutex> lock(work_mutex_);

  current_tick_ = tick;
  current_events_ = events;

  has_work_ = true;
  work_done_ = false;

  work_cv_.notify_one();
}

void Track::Join(AudioBuffer& output_buffer) {
  {
    std::unique_lock<std::mutex> lock(work_mutex_);
    done_cv_.wait(lock, [this]() { return work_done_ || stop_thread_; });

    if (stop_thread_) {
      return;
    }
  }

  // Now mix the processed audio into the output buffer
  auto ilch = track_buffer_.GetChannel(kLeftChannel);
  auto irch = track_buffer_.GetChannel(kRightChannel);
  auto olch = output_buffer.GetChannel(kLeftChannel);
  auto orch = output_buffer.GetChannel(kRightChannel);

  {
    std::scoped_lock<std::mutex> lock(mutex_);

    {
      for (int i = 0; i < track_buffer_.Size(); ++i) {
        SampleTick current_tick = current_tick_ + i;

        if (!settings_.muted_) {
          const float vol = settings_.volume_.GetValue(current_tick);
          const float pan = settings_.pan_.GetValue(current_tick);

          auto lgain = vol * LeftPan(pan);
          auto rgain = vol * RightPan(pan);

          olch[i] += ilch[i] * lgain;
          orch[i] += irch[i] * rgain;
        }
      }
    }
  }
}

absl::Status Track::ProcessLoop() {
  SOIR_TRACING_ZONE_COLOR("track::process_loop", SOIR_PINK);

  LOG(INFO) << "Track processing thread started for: " << settings_.name_;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(work_mutex_);
      work_cv_.wait(lock, [this]() { return has_work_ || stop_thread_; });

      if (stop_thread_) {
        break;
      }

      has_work_ = false;
    }

    {
      track_buffer_.Reset();

      {
        std::string trace_name = "track::render::" + inst_->GetName();
        SOIR_TRACING_ZONE_COLOR_STR(trace_name, SOIR_PINK);
        inst_->Render(current_tick_, current_events_, track_buffer_);
      }

      {
        SOIR_TRACING_ZONE_COLOR("track::render::fx-stack", SOIR_PINK);
        fx_stack_->Render(current_tick_, track_buffer_);
      }

      level_meter_.Process(track_buffer_.GetChannel(kLeftChannel),
                           track_buffer_.GetChannel(kRightChannel),
                           track_buffer_.Size());
    }

    {
      std::lock_guard<std::mutex> lock(work_mutex_);
      work_done_ = true;
      done_cv_.notify_one();
    }
  }

  LOG(INFO) << "Track processing thread stopped for: " << settings_.name_;

  return absl::OkStatus();
}

}  // namespace soir
