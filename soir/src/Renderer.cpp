#include "Renderer.hh"

#include <glog/logging.h>

#include "Constants.hh"

namespace maethstro {

constexpr int kStereoOutput = 2;

Renderer::Renderer() : currentBlock_(kStereoOutput, kBlockSize) {}

StatusOr<std::unique_ptr<Renderer>> Renderer::InitFromConfig(
    const Config& config, ILiquidPusher* pusher) {
  LOG(INFO) << "initializing renderer";

  auto renderer = std::make_unique<Renderer>();

  renderer->pusher_ = pusher;

  for (auto& trackCfg : config.tracks_) {
    std::unique_ptr<Track> track;
    MOVE_OR_RETURN(track, Track::LoadFromConfig(trackCfg));

    renderer->tracks_.push_back(std::move(track));
  }

  return renderer;
}

void Renderer::PushMessage(const juce::MidiMessage& msg, int sample) {
  const std::lock_guard<std::mutex> lock(midiBufferLock_);
  if (!midiBuffer_.addEvent(msg, sample)) {
    LOG(WARNING) << "unable to add MIDI event to buffer";
  }
}

Status Renderer::Loop(std::atomic<bool>& stop) {
  const auto sleepMs = std::chrono::milliseconds(static_cast<int>(
      (static_cast<double>(kBlockSize) / kSampleRate) * 1000.0));

  while (true) {
    auto startTime = std::chrono::high_resolution_clock::now();
    auto endTime = startTime + sleepMs;

    Status status = ProcessBlock();
    if (status != StatusCode::OK) {
      LOG(INFO) << "exiting the rendering loop with status=" << status;
      stop = true;
      return status;
    }

    if (stop) {
      LOG(INFO) << "exiting the rendering loop";
      return StatusCode::OK;
    }

    std::this_thread::sleep_until(endTime);
  }

  return StatusCode::OK;
}

Status Renderer::ProcessBlock() {
  {
    const std::lock_guard<std::mutex> lock(midiBufferLock_);
    midiBufferBlock_.swapWith(midiBuffer_);
  }

  currentBlock_.clear();

  if (!midiBufferBlock_.isEmpty()) {
    LOG(INFO) << "sending MIDI event to tracks, count="
              << midiBufferBlock_.getNumEvents();
  }

  for (auto& track : tracks_) {
    auto block = midiBufferBlock_;
    track->Render(block, currentBlock_);
  }

  pusher_->PushBlock(midiBufferBlock_, currentBlock_);

  midiBufferBlock_.clear();

  return StatusCode::OK;
}

}  // namespace maethstro
