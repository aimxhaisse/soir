#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <mutex>
#include <thread>

#include "IFaces.hh"
#include "Status.hh"
#include "Track.hh"

namespace maethstro {

struct Renderer : IMidiConsumer {
  static StatusOr<std::unique_ptr<Renderer>> InitFromConfig(
      const Config& config, ILiquidPusher* pusher);

  Status Loop(std::atomic<bool>& stop);

  void PushMessage(const juce::MidiMessage& msg, int sample);

  Renderer();
  virtual ~Renderer() {}

 private:
  Status ProcessBlock();

  std::vector<std::unique_ptr<Track>> tracks_;
  juce::AudioSampleBuffer currentBlock_;
  std::mutex midiBufferLock_;
  juce::MidiBuffer midiBuffer_;
  juce::MidiBuffer midiBufferBlock_;
  ILiquidPusher* pusher_;
};

}  // namespace maethstro
