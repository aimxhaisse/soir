#pragma once

#include <JuceHeader.h>

#include <list>
#include <memory>
#include <string>

#include "Config.hh"
#include "IFaces.hh"

namespace maethstro {

// Track to be rendered.
struct Track {
  static StatusOr<std::unique_ptr<Track>> LoadFromConfig(
      const TrackConfig& cfg);

  Track();

  void Render(juce::MidiBuffer& midi_block, juce::AudioSampleBuffer& into);

 private:
  juce::AudioSampleBuffer buffer_;
  std::unique_ptr<IPlugin> instr_;
  std::list<std::unique_ptr<IPlugin>> fxs_;
};

}  // namespace maethstro
