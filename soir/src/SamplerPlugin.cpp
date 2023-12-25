#include "SamplerPlugin.hh"

#include <glog/logging.h>

namespace maethstro {

SamplerPlugin::Sampler::Sampler(const SamplerEventConfig& config,
                                MemoryMappedAudioFormatReader* sample)
    : config_(config), isPlaying_(false), pos_(0), sample_(sample) {}

SamplerPlugin::SamplerPlugin(const SamplerConfig& config) : config_(config) {}

bool SamplerPlugin::Load() {
  LOG(INFO) << "looking for samples in " << config_.sampleDir_;
  auto dir = juce::File(config_.sampleDir_);
  if (!dir.isDirectory()) {
    LOG(ERROR) << "unable to initialize sampler because sample dir is not "
                  "valid, sample dir="
               << config_.sampleDir_;
    return false;
  }

  juce::WavAudioFormat fmt;

  for (auto sample :
       dir.findChildFiles(juce::File::findFiles, true, /* recursive search */
                          "*.wav")) {
    const std::string filename = sample.getFileName().toStdString();
    const std::string name = filename.substr(0, filename.size() - 4);

    for (auto& event : config_.events_) {
      LOG(INFO) << "checking if event=" << event.sampleName_ << " matches sample=" << name;
      if (event.sampleName_ == name) {
        LOG(INFO) << "trying to load sample=" << sample.getFileName();
        if (samplers_.count(name)) {
          LOG(ERROR) << "duplicate sample name found in sampler with name="
                     << name;
          return false;
        }
        samplers_[name].reset(
            new Sampler(event, fmt.createMemoryMappedReader(sample)));
        LOG(INFO) << "sample " << sample.getFileName()
                  << " properly loaded with name=" << name;

        if (!samplers_[name]->sample_->mapEntireFile()) {
          LOG(ERROR) << "unable to map sample file " << sample.getFileName();
          return false;
        }

        if (samplers_[name]->sample_->getChannelLayout() !=
            juce::AudioChannelSet::stereo()) {
          LOG(ERROR) << "sample " << sample.getFileName()
                     << " is not stereo, please convert it";
          return false;
        }
      }
    }
  }

  return true;
}

void SamplerPlugin::processBlock(juce::AudioBuffer<float>& buffer,
                                 juce::MidiBuffer& events) {
  const int samples = buffer.getNumSamples();

  bool const inputIsStereo = buffer.getNumChannels() == 2;

  if (!inputIsStereo) {
    LOG(FATAL) << "sample plugin input is not stereo, unsupported";
  }

  float wavSample[2];

  auto midiIt = events.begin();

  for (int sample = 0; sample < samples; ++sample) {
    while (midiIt != events.end() && (*midiIt).samplePosition == sample) {
      auto msg = (*midiIt).getMessage();

      for (auto& it : samplers_) {
        auto& sampler = it.second;

        if (sampler->config_.midiChan_ != msg.getChannel()) {
          continue;
        }
        if (sampler->config_.midiNote_ != msg.getNoteNumber()) {
          continue;
        }

        if (msg.isNoteOff()) {
          sampler->isPlaying_ = false;
          sampler->pos_ = 0;
          LOG(INFO) << "stopped playing sample for midi chan "
                    << sampler->config_.midiChan_;
        }

        if (msg.isNoteOn()) {
          sampler->pos_ = 0;
          sampler->isPlaying_ = true;
          LOG(INFO) << "playing sample for midi chan "
                    << sampler->config_.midiChan_;
        }
      }

      midiIt++;
    }

    for (auto& it : samplers_) {
      auto& sampler = it.second;

      // Handle playing.
      if (sampler->isPlaying_) {
        sampler->sample_->getSample(sampler->pos_, wavSample);

        float left = buffer.getSample(0, sample);
        float right = buffer.getSample(1, sample);

        left += wavSample[0];
        right += wavSample[1];

        buffer.setSample(0, sample, left);
        buffer.setSample(1, sample, right);

        sampler->pos_ += 1;
        sampler->pos_ %= sampler->sample_->lengthInSamples;

        if (sampler->pos_ == 0 && !sampler->config_.repeat_) {
          sampler->isPlaying_ = false;
        }
      }
    }
  }
}

}  // namespace maethstro
