#include <absl/log/log.h>

#include "core/dsp/engine.hh"
#include "neon.grpc.pb.h"

namespace neon {
namespace dsp {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing engine";

  block_size_ = config.Get<uint32_t>("neon.dsp.engine.block_size");
  current_tick_ = 0;

  auto http_enabled = config.Get<bool>("neon.dsp.output.http.enable");

  if (http_enabled) {
    http_server_ = std::make_unique<HttpServer>();
    auto status = http_server_->Init(config, this);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize HTTP server: " << status;
      return status;
    }
  }

  auto audio_output_enabled = config.Get<bool>("neon.dsp.output.audio.enable");

  if (audio_output_enabled) {
    audio_output_ = std::make_unique<AudioOutput>();
    auto status = audio_output_->Init(config);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize audio output: " << status;
      return status;
    }
  }

  sample_manager_ = std::make_unique<SampleManager>();
  auto status = sample_manager_->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize sample manager: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Engine::Start() {
  LOG(INFO) << "Starting engine";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Engine failed: " << status;
    }
  });

  if (audio_output_.get() != nullptr) {
    RegisterConsumer(audio_output_.get());

    auto status = audio_output_->Start();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to start audio output: " << status;
      return status;
    }
  }

  if (http_server_.get() != nullptr) {
    auto status = http_server_->Start();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to start HTTP server: " << status;
      return status;
    }
  }

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

  if (http_server_.get() != nullptr) {
    auto status = http_server_->Stop();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to stop HTTP server: " << status;
      return status;
    }
  }

  if (audio_output_.get() != nullptr) {
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

  thread_.join();

  for (auto& it : tracks_) {
    auto status = it.second->Stop();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to stop track: " << status;
    }
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

void Engine::Stats(const absl::Time& next_block_at,
                   const absl::Duration& block_duration) const {
  // Stupid simple implementation, only print some blocks time,
  // nothing about average or distributions for now. We might later
  // move to a more accurate implementation.
  auto now = absl::Now();
  auto lag = block_duration - (next_block_at - now);
  auto cpu_usage = (lag / block_duration) * 100.0;

  LOG_EVERY_N_SEC(INFO, 10)
      << "\e[1;104mS O I R\e[0m CPU usage: " << cpu_usage << "%";
}

void Engine::SetTicks(std::list<MidiEventAt>& events) {
  auto now = absl::Now();

  for (auto& e : events) {
    auto diff_us = absl::ToInt64Microseconds(e.At() - now);
    auto diff_ticks = static_cast<int32_t>((diff_us * kSampleRate) / 1e6);

    // We introduce an artificial delay here that is greater than the
    // block size to ensure we have enough time for processing until
    // it is actually scheduled.
    diff_ticks += kBlockDelay * block_size_;

    diff_ticks = std::max(diff_ticks, 0);

    e.SetTick(current_tick_ + diff_ticks);
  }
}

void Engine::PushMidiEvent(const MidiEventAt& e) {
  std::scoped_lock<std::mutex> lock(msgs_mutex_);

  msgs_by_track_[e.Track()].push_back(e);
}

absl::Status Engine::Run() {
  LOG(INFO) << "Engine running";

  AudioBuffer buffer(block_size_);
  absl::Duration block_duration =
      absl::Microseconds((1e6 * block_size_) / kSampleRate);
  absl::Time next_block_at = absl::Now();

  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, absl::ToChronoTime(next_block_at),
                     [this]() { return stop_; });
      if (stop_) {
        break;
      }
    }

    std::map<int, std::list<MidiEventAt>> events;
    {
      std::lock_guard<std::mutex> lock(msgs_mutex_);
      events.swap(msgs_by_track_);
    }

    buffer.Reset();
    {
      std::scoped_lock<std::mutex> lock(tracks_mutex_);
      for (auto& it : tracks_) {
        auto track = it.second.get();
        auto evlist = events[track->GetTrackId()];
        SetTicks(evlist);
        track->Render(current_tick_, evlist, buffer);
      }
    }

    current_tick_ += block_size_;

    {
      std::lock_guard<std::mutex> lock(consumers_mutex_);
      for (auto consumer : consumers_) {
        auto status = consumer->PushAudioBuffer(buffer);
        if (!status.ok()) {
          LOG(WARNING) << "Failed to push samples to consumer: " << status;
        }
      }
    }

    next_block_at += block_duration;
    Stats(next_block_at, block_duration);
  }

  return absl::OkStatus();
}

absl::Status Engine::GetTracks(std::list<TrackSettings>* response) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);

  for (auto& it : tracks_) {
    auto track = it.second.get();

    response->push_back(track->GetSettings());
  }

  return absl::OkStatus();
}

absl::Status Engine::SetupTracks(const std::list<TrackSettings>& settings) {
  // Make sure we don't have concurrent calls here because the
  // following design described below is not atomic.
  std::scoped_lock<std::mutex> setup_lock(setup_tracks_mutex_);

  // We have here a somewhat complex design: initializing a track can
  // take time as we may need to load samples from disk. We don't want
  // to block the engine thread for that. So we take twice the tracks
  // lock: 1st time to know what we have to do (add new tracks,
  // initialize instruments, initialize effects, ...), then we prepare
  // everything, and take the lock to update the tracks with
  // everything pre-loaded.

  // Use maps here to ensure we don't override the same track multiple
  // times.
  std::map<int, TrackSettings> tracks_to_add;
  std::map<int, TrackSettings> tracks_to_update;

  // Check what we need to do.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& track_settings : settings) {
      auto track_id = track_settings.track_;
      auto it = tracks_.find(track_id);

      if (it == tracks_.end() || !it->second->CanFastUpdate(track_settings)) {
        tracks_to_add[track_id] = track_settings;
      } else {
        tracks_to_update[track_id] = track_settings;
      }
    }
  }

  std::map<int, std::unique_ptr<Track>> updated_tracks;

  // Perform slow operations here.
  for (auto& track : tracks_to_add) {
    auto new_track = std::make_unique<Track>(block_size_);
    auto status = new_track->Init(track.second, sample_manager_.get());
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize track: " << status;
      return status;
    }

    updated_tracks[track.first] = std::move(new_track);
  }

  // Update the layout without holding the lock for too long.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& track_request : tracks_to_update) {
      auto track_id = track_request.first;
      auto& track = tracks_[track_id];

      // This can't fail otherwise the design is not atomic, we don't
      // want partial upgrades to be possible.
      track->FastUpdate(track_request.second);

      updated_tracks[track_id] = std::move(track);
    }

    tracks_.swap(updated_tracks);
  }

  return absl::OkStatus();
}

SampleManager& Engine::GetSampleManager() {
  return *sample_manager_;
}

}  // namespace dsp
}  // namespace neon
