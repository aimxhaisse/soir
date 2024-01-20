#include <absl/log/log.h>

#include "engine.hh"

namespace maethstro {
namespace soir {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const common::Config& config) {
  LOG(INFO) << "Initializing engine";

  block_size_ = config.Get<uint32_t>("soir.engine.block_size");

  for (auto& track_config : config.GetConfigs("soir.tracks")) {
    std::unique_ptr<Track> track = std::make_unique<Track>();

    auto status = track->Init(*track_config);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to initialize track: " << status;
      return status;
    }

    tracks_.push_back(std::move(track));
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
    for (auto& track : tracks_) {
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

  for (auto& track : tracks_) {
    auto* track_response = response->add_tracks();

    track_response->set_channel(track->GetChannel());
    track_response->set_instrument(proto::Track::TRACK_MONO_SAMPLER);
    track_response->set_volume(track->GetVolume());
    track_response->set_pan(track->GetPan());
    track_response->set_muted(track->IsMuted());
  }

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
