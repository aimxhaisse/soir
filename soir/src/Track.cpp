#include "Track.hh"

#include <glog/logging.h>

#include "Constants.hh"
#include "SamplerPlugin.hh"
#include "Vst3Plugin.hh"

namespace maethstro {

Track::Track() : buffer_(2, kBlockSize) {}

StatusOr<std::unique_ptr<Track>> Track::LoadFromConfig(const TrackConfig& cfg) {
  LOG(INFO) << "creating new track";
  auto track = std::make_unique<Track>();

  LOG(INFO) << "trying to load instrument of type " << cfg.instr_.type_;

  // VST3 instrument.
  if (cfg.instr_.type_ == kPluginTypeVst3) {
    std::unique_ptr<Vst3Plugin> instr =
        std::make_unique<Vst3Plugin>(cfg.instr_.vst3_.path_);

    LOG(INFO) << "loading vst3 instrument";
    if (!instr->Load(true)) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "unable to load vst3 instrument '" << instr->path_ << "'");
    }
    if (!instr->LoadPreset(cfg.instr_.vst3_.preset_)) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG, "unable to load vst3 preset '"
                                                   << cfg.instr_.vst3_.preset_
                                                   << "'");
    }
    LOG(INFO) << "instrument vst3 loaded";
    track->instr_ = std::move(instr);
  }

  // Sampler instrument.
  if (cfg.instr_.type_ == kPluginTypeSampler) {
    std::unique_ptr<SamplerPlugin> instr =
        std::make_unique<SamplerPlugin>(cfg.instr_.sampler_);

    LOG(INFO) << "loading sampler instrument";
    if (!instr->Load()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "unable to load sampler instrument");
    }
    LOG(INFO) << "instrument sampler loaded";
    track->instr_ = std::move(instr);
  }

  if (!track->instr_) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG, "no instrument found for track");
  }

  for (const auto& fxCfg : cfg.fxs_) {
    LOG(INFO) << "loading effect";
    std::unique_ptr<Vst3Plugin> fx = std::make_unique<Vst3Plugin>(fxCfg.path_);
    if (!fx->Load(false)) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "unable to load fx '" << fxCfg.path_ << "'");
    }
    if (!fx->LoadPreset(fxCfg.preset_)) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "unable to load preset '" << fxCfg.preset_ << "'");
    }
    track->fxs_.push_back(std::move(fx));
    LOG(INFO) << "effect loaded";
  }

  return track;
}

void Track::Render(juce::MidiBuffer& midi_block,
                   juce::AudioSampleBuffer& into) {
  buffer_.clear();
  instr_->processBlock(buffer_, midi_block);

  for (auto it = fxs_.begin(); it != fxs_.end(); ++it) {
    (*it)->processBlock(buffer_, midi_block);
  }

  into.addFrom(0, 0, buffer_, 0, 0, buffer_.getNumSamples());
  into.addFrom(1, 0, buffer_, 1, 0, buffer_.getNumSamples());
}

}  // namespace maethstro
