#pragma once

#include <JuceHeader.h>

#include <SFML/Network.hpp>
#include <atomic>
#include <cstdio>
#include <mutex>

#include "Config.hh"
#include "IFaces.hh"
#include "Shouter.hh"
#include "Status.hh"

namespace maethstro {

struct LiquidPusher : public ILiquidPusher {
  virtual ~LiquidPusher() {}

  explicit LiquidPusher(const Config& config_);

  static StatusOr<std::unique_ptr<LiquidPusher>> InitFromConfig(
      const Config& config);

  Status Loop(std::atomic<bool>& stop);
  void PushBlock(const juce::MidiBuffer& midi,
                 const juce::AudioSampleBuffer& block);

 private:
  const Config& config_;
  std::mutex blocksLock_;
  std::list<juce::AudioSampleBuffer> blocks_;
  std::list<juce::MidiMessage> midiSysExMsg_;
};

}  // namespace maethstro
