#include <absl/log/log.h>

#include "engine.hh"

namespace maethstro {
namespace soir {

Engine::Engine() {}

Engine::~Engine() {}

absl::Status Engine::Init(const common::Config& config) {
  LOG(INFO) << "Initializing engine";

  sample_rate_ = config.Get<uint32_t>("soir.engine.sample_rate");
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

  std::unique_lock<std::mutex> lock(mutex_);
  consumers_.push_back(consumer);
}

void Engine::RemoveConsumer(SampleConsumer* consumer) {
  LOG(INFO) << "Removing engine consumer";

  std::unique_lock<std::mutex> lock(mutex_);
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

absl::Status Engine::Run() {
  LOG(INFO) << "Engine running";

  std::vector<float> samples(block_size_);
  absl::Time next_block_at = absl::Now();
  absl::Duration block_duration =
      absl::Microseconds(1000000 * block_size_ / sample_rate_);

  while (true) {

    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, absl::ToChronoTime(next_block_at),
                     [this]() { return stop_; });
      if (stop_) {
        break;
      }
    }

    for (auto consumer : consumers_) {
      auto status = consumer->PushSamples(samples);
      if (!status.ok()) {
        LOG(WARNING) << "Failed to push samples to consumer: " << status;
      }
    }

    std::fill(samples.begin(), samples.end(), 0);
    next_block_at += block_duration;
    Stats(next_block_at, block_duration);
  }

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
