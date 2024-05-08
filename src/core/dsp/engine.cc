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

  http_server_ = std::make_unique<HttpServer>();
  auto status = http_server_->Init(config, this);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize HTTP server: " << status;
    return status;
  }

  sample_manager_ = std::make_unique<SampleManager>();
  status = sample_manager_->Init(config);
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

  auto status = http_server_->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start HTTP server: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

  auto status = http_server_->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop HTTP server: " << status;
    return status;
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cv_.notify_all();
  }

  thread_.join();

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

void Engine::PushMidiEvent(const libremidi::message& msg) {

  if (msg.get_message_type() == libremidi::message_type::SYSTEM_EXCLUSIVE) {
    proto::MidiSysexInstruction sysex;
    if (!sysex.ParseFromArray(msg.bytes.data() + 1, msg.bytes.size() - 1)) {
      LOG(WARNING) << "Failed to parse sysex message";
      return;
    }

    {
      std::scoped_lock<std::mutex> lock(msgs_mutex_);
      msgs_by_chan_[sysex.channel()].push_back(msg);
    }

    return;
  }

  // Here we assume it has a channel, but it may not be the case, we
  // should filter by message types that support channels.

  if (msg.get_channel() != 0) {
    std::scoped_lock<std::mutex> lock(msgs_mutex_);
    msgs_by_chan_[msg.get_channel()].push_back(msg);
    return;
  }
}

absl::Status Engine::Run() {
  LOG(INFO) << "Engine running";

  AudioBuffer buffer(block_size_);
  absl::Time next_block_at = absl::Now();
  absl::Duration block_duration =
      absl::Microseconds(1000000 * block_size_ / kSampleRate);

  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, absl::ToChronoTime(next_block_at),
                     [this]() { return stop_; });
      if (stop_) {
        break;
      }
    }

    std::map<int, std::list<libremidi::message>> events;
    {
      std::lock_guard<std::mutex> lock(msgs_mutex_);
      events.swap(msgs_by_chan_);
    }

    buffer.Reset();
    {
      std::scoped_lock<std::mutex> lock(tracks_mutex_);
      for (auto& it : tracks_) {
        auto track = it.second.get();
        track->Render(events[track->GetChannel()], buffer);
      }
    }

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
      auto channel = track_settings.channel_;
      auto it = tracks_.find(channel);

      if (it == tracks_.end() || !it->second->CanFastUpdate(track_settings)) {
        tracks_to_add[channel] = track_settings;
      } else {
        tracks_to_update[channel] = track_settings;
      }
    }
  }

  std::map<int, std::unique_ptr<Track>> updated_tracks;

  // Perform slow operations here.
  for (auto& track : tracks_to_add) {
    auto new_track = std::make_unique<Track>();
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
      auto channel = track_request.first;
      auto& track = tracks_[channel];

      // This can't fail otherwise the design is not atomic, we don't
      // want partial upgrades to be possible.
      track->FastUpdate(track_request.second);

      updated_tracks[channel] = std::move(track);
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
