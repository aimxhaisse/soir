#pragma once

#include <JuceHeader.h>

#include <map>

#include "Config.hh"
#include "IFaces.hh"

namespace maethstro {

struct SamplerPlugin : public IPlugin {
  explicit SamplerPlugin(const SamplerConfig& config);
  bool Load();

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& msgs) override;

 private:
  struct Sampler {
    Sampler(const SamplerEventConfig& config,
            juce::MemoryMappedAudioFormatReader* sample);

    SamplerEventConfig config_;
    bool isPlaying_;
    int pos_;
    std::unique_ptr<juce::MemoryMappedAudioFormatReader> sample_;
  };

  const SamplerConfig& config_;

  // For now, only support playing the same sample source at once,
  // this means we need to duplicate files with different names if we
  // want to use a sample twice. That's fine for now as we can have
  // sorts of 'pack folders' for a given song, structured like the
  // song instead of like the pack of samples used.
  std::map<std::string, std::unique_ptr<Sampler>> samplers_;
};

}  // namespace maethstro
