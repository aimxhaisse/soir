#pragma once

#include <JuceHeader.h>

#include <iostream>
#include <string>

#include "IFaces.hh"

namespace maethstro {

struct PresetLoader : public juce::ExtensionsVisitor {
  bool load(const std::string& in);

  virtual void visitVST3Client(const VST3Client& client);

 private:
  juce::MemoryBlock preset_;
};

struct Vst3Plugin : public IPlugin {
  explicit Vst3Plugin(const std::string& path);
  bool Load(bool is_fx);
  bool LoadPreset(const std::string& path);

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& msgs) override;

  const std::string path_;
  juce::AudioPluginFormatManager fmt_;
  std::unique_ptr<juce::AudioPluginInstance> plugin_;
};

}  // namespace maethstro
