#include <absl/log/log.h>

#include "engine.hh"

namespace maethstro {
namespace soir {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const common::Config& config) {
  LOG(INFO) << "Initializing engine";

  block_size_ = config.Get<uint32_t>("soir.engine.block_size");

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

  return absl::OkStatus();
}

absl::Status Engine::Stop() {
  LOG(INFO) << "Stopping engine";

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

void Engine::PushMidiEvent(const proto::MidiEvents_Request& event) {
  libremidi::message msg;
  msg.bytes = libremidi::midi_bytes(event.midi_payload().begin(),
                                    event.midi_payload().end());

  std::scoped_lock<std::mutex> lock(msgs_mutex_);
  msgs_by_chan_[msg.get_channel()].push_back(msg);
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
    for (auto& it : tracks_) {
      auto track = it.second.get();
      track->Render(events[track->GetChannel()], buffer);
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

absl::Status Engine::GetTracks(proto::GetTracks_Response* response) {
  std::scoped_lock<std::mutex> lock(tracks_mutex_);

  for (auto& it : tracks_) {
    auto track = it.second.get();
    auto* track_response = response->add_tracks();

    track_response->set_channel(track->GetChannel());
    track_response->set_instrument(track->GetInstrument());
    track_response->set_volume(track->GetVolume());
    track_response->set_pan(track->GetPan());
    track_response->set_muted(track->IsMuted());
  }

  return absl::OkStatus();
}

absl::Status Engine::SetupTracks(const proto::SetupTracks_Request* request) {
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

  std::list<proto::Track> tracks_to_add;
  std::list<proto::Track> tracks_to_update;

  // Check what we need to do.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (int i = 0; i < request->tracks_size(); ++i) {
      auto& track_request = request->tracks(i);
      int channel = track_request.channel();
      auto it = tracks_.find(channel);

      if (it == tracks_.end() || !it->second->CanFastUpdate(track_request)) {
        tracks_to_add.push_back(track_request);
      } else {
        tracks_to_update.push_back(track_request);
      }
    }
  }

  std::map<int, std::unique_ptr<Track>> updated_tracks;

  // Perform slow operations here.
  for (auto& track : tracks_to_add) {
    auto new_track = std::make_unique<Track>();
    auto status = new_track->Init(track);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize track: " << status;
      return status;
    }

    updated_tracks[track.channel()] = std::move(new_track);
  }

  // Update the layout without holding the lock for too long.
  {
    std::scoped_lock<std::mutex> lock(tracks_mutex_);
    for (auto& track_request : tracks_to_update) {
      auto channel = track_request.channel();
      auto& track = tracks_[channel];

      // This can't fail otherwise the design is not atomic, we don't
      // want partial upgrades to be possible.
      track->FastUpdate(track_request);

      updated_tracks[channel] = std::move(track);
    }

    tracks_.swap(updated_tracks);
  }

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
