#include "LiquidPusher.hh"

#include <glog/logging.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

#include "Constants.hh"
#include "VorbisWriter.hh"

namespace maethstro {

LiquidPusher::LiquidPusher(const Config& config) : config_(config) {}

StatusOr<std::unique_ptr<LiquidPusher>> LiquidPusher::InitFromConfig(
    const Config& config) {
  LOG(INFO) << "initializing liquid pusher";

  auto service = std::make_unique<LiquidPusher>(config);

  return service;
}

Status LiquidPusher::Loop(std::atomic<bool>& stop) {
  std::list<juce::AudioSampleBuffer> blocks;
  std::list<juce::MidiMessage> messages;
  juce::MemoryOutputStream memoryStream;
  Shouter shouter;

  LOG(INFO) << "starting liquid pusher loop";

  Status code = shouter.Init(config_);
  RETURN_IF_ERROR(code);

  VorbisWriter vorbisWriter(&memoryStream);

  // Initialize with an empty metadata block.
  code = vorbisWriter.Reset("{}");
  RETURN_IF_ERROR(code);

  const auto sleepMs = std::chrono::milliseconds(static_cast<int>(
      (static_cast<double>(kBlockSize) / kSampleRate) * 1000.0));

  while (true) {
    auto startTime = std::chrono::high_resolution_clock::now();
    auto endTime = startTime + sleepMs;

    {
      const std::lock_guard<std::mutex> lock(blocksLock_);
      blocks.swap(blocks_);
      messages.swap(midiSysExMsg_);
    }

    // Only send the last metadata update we have, there is no point
    // in resetting the stream twice.
    if (!messages.empty()) {
      auto& midiMsg = messages.back();
      const std::string sysex(
          reinterpret_cast<const char*>(midiMsg.getSysExData()),
          static_cast<std::size_t>(midiMsg.getSysExDataSize()));
      code = vorbisWriter.Reset(sysex);
      RETURN_IF_ERROR(code);
      messages.clear();
    }

    for (auto& block : blocks) {
      vorbisWriter.writeFromAudioSampleBuffer(block, 0, block.getNumSamples());
    }
    blocks.clear();

    // "If the format supports flushing and the operation succeeds, this returns
    // true."; let's ignore errors here.
    vorbisWriter.flush();

    if (memoryStream.getDataSize() > 0) {
      auto status =
          shouter.Send(memoryStream.getData(), memoryStream.getDataSize());
      if (status != StatusCode::OK) {
        LOG(WARNING) << status;
        return status;
      }
      memoryStream.reset();
    }

    if (stop) {
      LOG(INFO) << "exiting the liquid pusher loop";
      shouter.Close();
      return StatusCode::OK;
    }

    std::this_thread::sleep_until(endTime);
  }

  return StatusCode::OK;
}

void LiquidPusher::PushBlock(const juce::MidiBuffer& midiBuffer,
                             const juce::AudioSampleBuffer& block) {
  const std::lock_guard<std::mutex> lock(blocksLock_);
  blocks_.push_back(block);

  for (auto it = midiBuffer.begin(); it != midiBuffer.end(); ++it) {
    auto msg = (*it).getMessage();

    if (msg.isSysEx()) {
      LOG(INFO) << "pushed SysEx message";
      midiSysExMsg_.push_back(msg);
    }
  }
}

}  // namespace maethstro
