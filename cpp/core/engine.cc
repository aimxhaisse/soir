#include "core/engine.hh"

#include <absl/log/log.h>

#include "audio/audio_recorder.hh"
#include "vst/vst_host.hh"

namespace soir {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing engine";

  current_tick_ = 0;

  audio_output_enabled_ = config.Get<bool>("dsp.enable_output");

  audio_output_ = std::make_unique<audio::AudioOutput>();
  if (audio_output_enabled_) {
    auto status = audio_output_->Init(kSampleRate, kNumChannels, kBlockSize);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize audio output: " << status;
      return status;
    }
  } else {
    LOG(INFO) << "Audio output disabled";
  }

  sample_manager_ = std::make_unique<SampleManager>();
  auto status = sample_manager_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize sample manager: " << status;
    return status;
  }
  LOG(INFO) << "Sample manager initialized";

  controls_ = std::make_unique<Controls>();
  status = controls_->Init();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize controls: " << status;
    return status;
  }
  LOG(INFO) << "Controls initialized";

  vst_host_ = std::make_unique<vst::VstHost>();
  status = vst_host_->Init();
  if (!status.ok()) {
    LOG(WARNING) << "Failed to initialize VST host: " << status;
  } else {
    status = vst_host_->ScanPlugins();
    if (!status.ok()) {
      LOG(WARNING) << "Failed to scan VST plugins: " << status;
    }
  }

  audio_recorder_ = std::make_unique<AudioRecorder>();

  return absl::OkStatus();
}

Controls* Engine::GetControls() { return controls_.get(); }

vst::VstHost* Engine::GetVstHost() { return vst_host_.get(); }

absl::Status Engine::Start() {
  LOG(INFO) << "Starting engine";

  // We do not start tracks here as there is no track at the init of
  // the engine: tracks are added through the SetupTracks method from
  // Python.

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Engine failed: " << status;
    }
  });

  if (audio_output_.get() != nullptr && audio_output_enabled_) {
    RegisterConsumer(audio_output_.get());

    auto status = audio_output_->Start();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to start audio output: " << status;
      return status;
    }
  }

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

  if (audio_output_.get() != nullptr && audio_output_enabled_) {
    auto status = audio_output_->Stop();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to stop audio output: " << status;
      return status;
    }

    RemoveConsumer(audio_output_.get());
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cv_.notify_all();
  }

  if (thread_.joinable()) {
    thread_.join();
  }

  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& it : tracks_) {
      auto status = it.second->Stop();
      if (!status.ok()) {
        LOG(ERROR) << "Failed to stop track thread: " << status;
      }
    }
    tracks_.clear();
  }

  LOG(INFO) << "Engine stopped";

  return absl::OkStatus();
}

void Engine::RegisterConsumer(SampleConsumer* consumer) {
  LOG(INFO) << "Registering engine consumer";

  std::scoped_lock<std::mutex> lock(consumers_mutex_);
  consumers_.push_back(consumer);
}

void Engine::RemoveConsumer(SampleConsumer* consumer) {
  LOG(INFO) << "Removing engine consumer";

  std::scoped_lock<std::mutex> lock(consumers_mutex_);
  consumers_.remove(consumer);
}

void Engine::SetTicks(std::list<MidiEventAt>& events) {
  auto now = absl::Now();

  for (auto& e : events) {
    auto diff_us = absl::ToInt64Microseconds(e.At() - now);
    auto diff_ticks = static_cast<int32_t>((diff_us * kSampleRate) / 1e6);

    // We introduce an artificial delay here that is greater than the
    // block size to ensure we have enough time for processing until
    // it is actually scheduled.
    diff_ticks += kBlockProcessingDelay * kBlockSize;

    diff_ticks = std::max(diff_ticks, 0);

    e.SetTick(current_tick_ + diff_ticks);
  }
}

void Engine::PushMidiEvent(const MidiEventAt& e) {
  std::scoped_lock<std::mutex> lock(msgs_mutex_);

  msgs_by_track_[e.Track()].push_back(e);
}

absl::Status Engine::Run() {
  SOIR_TRACING_ZONE_COLOR("dsp::run", SOIR_BLUE);

  LOG(INFO) << "Engine running";

  AudioBuffer buffer(kBlockSize);
  absl::Duration block_duration =
      absl::Microseconds((1e6 * kBlockSize) / kSampleRate);
  absl::Time next_block_at = absl::Now();
  absl::Time initial_time = next_block_at;
  uint64_t block_count = 0;

  while (true) {
    {
      SOIR_TRACING_ZONE_COLOR("dsp::wait", SOIR_BLUE);

      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, absl::ToChronoTime(next_block_at),
                     [this]() { return stop_; });
      if (stop_) {
        break;
      }
    }

    std::map<std::string, std::list<MidiEventAt>> events;
    {
      SOIR_TRACING_ZONE_COLOR("dsp:swap-midi", SOIR_BLUE);

      std::lock_guard<std::mutex> lock(msgs_mutex_);
      events.swap(msgs_by_track_);
    }

    // Update knobs prior to rendering so it uses up-to-date values.
    //
    // This is important as some of the DSP code can be bound to the
    // knob values which aren't yet created.
    {
      SOIR_TRACING_ZONE_COLOR("dsp::controls-update", SOIR_BLUE);

      auto evlist = events[std::string(kInternalControls)];
      SetTicks(evlist);
      controls_->AddEvents(evlist);
      controls_->Update(current_tick_);
    }

    {
      SOIR_TRACING_ZONE_COLOR("dsp::tracks-async-render", SOIR_BLUE);

      // Kick off all track rendering operations in parallel
      {
        std::scoped_lock<std::mutex> lock(tracks_mutex_);
        for (auto& it : tracks_) {
          auto track = it.second.get();
          auto evlist = events[track->GetTrackName()];
          SetTicks(evlist);
          track->RenderAsync(current_tick_, evlist);
        }
      }

      // Reset the output buffer before collecting results
      buffer.Reset();

      // Join all track rendering operations, order is not important
      // as it's just an addition (TRACK(A) + TRACK(B) = TRACK(B) +
      // TRACK(A)).
      {
        SOIR_TRACING_ZONE_COLOR("dsp::tracks-join", SOIR_BLUE);
        std::scoped_lock<std::mutex> lock(tracks_mutex_);
        for (auto& it : tracks_) {
          auto track = it.second.get();
          track->Join(buffer);
        }
      }

      master_meter_.Process(buffer.GetChannel(kLeftChannel),
                            buffer.GetChannel(kRightChannel), buffer.Size());
    }

    current_tick_ += kBlockSize;

    {
      SOIR_TRACING_ZONE_COLOR("dsp::output", SOIR_BLUE);
      std::lock_guard<std::mutex> lock(consumers_mutex_);
      for (auto consumer : consumers_) {
        auto status = consumer->PushAudioBuffer(buffer);
        if (!status.ok()) {
          LOG(WARNING) << "Failed to push samples to consumer: " << status;
        }
      }
    }

    block_count++;
    next_block_at = initial_time + block_count * block_duration;

    SOIR_TRACING_FRAME("dsp::frame");
  }

  return absl::OkStatus();
}

absl::Status Engine::GetTracks(std::list<Track::Settings>* response) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);

  for (auto& it : tracks_) {
    auto track = it.second.get();

    response->push_back(track->GetSettings());
  }

  return absl::OkStatus();
}

absl::Status Engine::SetupTracks(const std::list<Track::Settings>& settings) {
  // Make sure we don't have concurrent calls here because the
  // following design described below is not atomic.
  std::scoped_lock<std::mutex> setup_lock(setup_tracks_mutex_);

  // We have here a somewhat complex design: initializing a track can
  // take time We don't want to block the engine thread for that. So
  // we take twice the tracks lock: 1st time to know what we have to
  // do (add new tracks, initialize instruments, initialize effects,
  // ...), then we prepare everything, and take the lock to update the
  // tracks with everything pre-loaded.

  // Use maps here to ensure we don't override the same track multiple
  // times.
  std::map<std::string, Track::Settings> tracks_to_add;
  std::map<std::string, Track::Settings> tracks_to_update;

  // Check what we need to do.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& track_settings : settings) {
      auto name = track_settings.name_;
      auto it = tracks_.find(name);

      if (it == tracks_.end() || !it->second->CanFastUpdate(track_settings)) {
        tracks_to_add[name] = track_settings;
      } else {
        tracks_to_update[name] = track_settings;
      }
    }
  }

  std::map<std::string, std::unique_ptr<Track>> updated_tracks;

  // Perform slow operations here.
  for (auto& track : tracks_to_add) {
    auto new_track = std::make_unique<Track>();
    auto status = new_track->Init(track.second, sample_manager_.get(),
                                  controls_.get(), vst_host_.get());
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize track: " << status;
      return status;
    }

    // Start the track's processing thread
    status = new_track->Start();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to start new track thread: " << status;
      return status;
    }

    updated_tracks[track.first] = std::move(new_track);
  }

  // Update the layout without holding the lock for too long.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& track_request : tracks_to_update) {
      auto name = track_request.first;
      auto& track = tracks_[name];

      // This can't fail otherwise the design is not atomic, we don't
      // want partial upgrades to be possible.
      track->FastUpdate(track_request.second);

      updated_tracks[name] = std::move(track);
    }

    tracks_.swap(updated_tracks);
  }

  return absl::OkStatus();
}

SampleManager& Engine::GetSampleManager() { return *sample_manager_; }

Levels Engine::GetMasterLevels() const { return master_meter_.GetLevels(); }

std::optional<Levels> Engine::GetTrackLevels(const std::string& name) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);
  auto it = tracks_.find(name);
  if (it != tracks_.end()) {
    return it->second->GetLevels();
  }
  return std::nullopt;
}

absl::Status Engine::OpenVstEditor(const std::string& track_name,
                                   const std::string& fx_name) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);
  auto it = tracks_.find(track_name);
  if (it == tracks_.end())
    return absl::NotFoundError("Track not found: " + track_name);
  return it->second->OpenVstEditor(fx_name);
}

absl::Status Engine::CloseVstEditor(const std::string& track_name,
                                    const std::string& fx_name) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);
  auto it = tracks_.find(track_name);
  if (it == tracks_.end())
    return absl::NotFoundError("Track not found: " + track_name);
  return it->second->CloseVstEditor(fx_name);
}

absl::Status Engine::StartRecording(const std::string& file_path) {
  if (!audio_recorder_) {
    return absl::InternalError("AudioRecorder not initialized");
  }

  auto status = audio_recorder_->Init(file_path);
  if (!status.ok()) {
    return status;
  }

  RegisterConsumer(audio_recorder_.get());

  LOG(INFO) << "Started recording to: " << file_path;
  return absl::OkStatus();
}

absl::Status Engine::StopRecording() {
  if (!audio_recorder_) {
    return absl::InternalError("AudioRecorder not initialized");
  }

  RemoveConsumer(audio_recorder_.get());

  auto status = audio_recorder_->MaybeStop();
  if (!status.ok()) {
    return status;
  }

  LOG(INFO) << "Stopped recording";
  return absl::OkStatus();
}

}  // namespace soir
