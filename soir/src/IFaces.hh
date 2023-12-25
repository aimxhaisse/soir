#pragma once

#include <JuceHeader.h>

namespace maethstro {
struct IMidiConsumer {
  virtual void PushMessage(const juce::MidiMessage& msg, int sample) = 0;
};

struct ILiquidPusher {
  virtual void PushBlock(const juce::MidiBuffer& midi,
                         const juce::AudioSampleBuffer& block) = 0;
};

struct IPlugin {
  virtual void processBlock(juce::AudioBuffer<float>& buffer,
                            juce::MidiBuffer& msgs) = 0;
};
}  // namespace maethstro
