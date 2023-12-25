#include "Vst3Plugin.hh"

#include <glog/logging.h>

#include "Constants.hh"

namespace maethstro {

bool PresetLoader::load(const std::string& in) {
  juce::File file(in);

  return file.loadFileAsData(preset_);
}

void PresetLoader::visitVST3Client(const VST3Client& client) {
  LOG(INFO) << "setting preset to VST3 client";
  if (!client.setPreset(preset_)) {
    LOG(WARNING) << "unable to set preset to VST3 client";
  } else {
    LOG(WARNING) << "VST3 preset was properly set";
  }
}

Vst3Plugin::Vst3Plugin(const std::string& path) : path_(path) {
  fmt_.addDefaultFormats();
}

bool Vst3Plugin::Load(bool is_fx) {
  juce::OwnedArray<PluginDescription> descriptions;
  juce::KnownPluginList plugins;

  for (int i = 0; i < fmt_.getNumFormats(); ++i) {
    juce::AudioPluginFormat* fmt = fmt_.getFormat(i);
    LOG(INFO) << "looking for " << fmt->getName();
    plugins.scanAndAddFile(path_, true, descriptions, *fmt);
  }

  if (descriptions.size() == 0) {
    LOG(WARNING) << "unable to load plugin " << path_;
    return false;
  }

  LOG(INFO) << "found VST3 plugin " << descriptions[0];

  juce::String err;
  plugin_ = fmt_.createPluginInstance(*descriptions[0], maethstro::kSampleRate,
                                      maethstro::kBlockSize, err);
  if (!plugin_) {
    LOG(WARNING) << "unable to load plugin, err=" << err;
    return false;
  }

  juce::AudioPluginInstance::BusesLayout out_layout;
  if (is_fx) {
    out_layout.inputBuses.add(AudioChannelSet::stereo());
  }
  out_layout.outputBuses.add(AudioChannelSet::stereo());

  plugin_->setBusesLayout(out_layout);
  plugin_->prepareToPlay(maethstro::kSampleRate, maethstro::kBlockSize);
  plugin_->setNonRealtime(false);

  return true;
}

bool Vst3Plugin::LoadPreset(const std::string& path) {
  PresetLoader preset;

  if (!preset.load(path)) {
    return false;
  }

  plugin_->getExtensions(preset);

  return true;
}

void Vst3Plugin::processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& msgs) {
  plugin_->processBlock(buffer, msgs);
}

}  // namespace maethstro
