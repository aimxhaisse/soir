#include "Config.hh"

#include <glog/logging.h>

#include "Constants.hh"

namespace maethstro {

StatusOr<std::unique_ptr<Config>> Config::LoadFromParser(ConfigParser& parser) {
  LOG(INFO) << "loading maethstro config";

  // Basic configuration.
  auto config = std::make_unique<Config>();
  config->port_ = parser.Get<int>("maethstro.port");
  if (!config->port_) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'port' at maethstro.port");
  }
  config->host_ = parser.Get<std::string>("maethstro.host");
  if (config->host_.empty()) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'host' at maethstro.host");
  }
  config->bpm_ = parser.Get<int>("maethstro.bpm");
  if (!config->bpm_) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'bpm' at maethstro.bpm");
  }

  // Liquid configuration.
  config->liquidHost_ = parser.Get<std::string>("maethstro.liquid.host");
  if (config->liquidHost_.empty()) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'host' at maethstro.liquid");
  }
  config->liquidMount_ = parser.Get<std::string>("maethstro.liquid.mount");
  if (config->liquidMount_.empty()) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'mount' at maethstro.liquid");
  }
  config->liquidPort_ = parser.Get<int>("maethstro.liquid.port");
  if (!config->liquidPort_) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'port' at maethstro.liquid");
  }
  config->liquidPassword_ =
      parser.Get<std::string>("maethstro.liquid.password");
  if (config->liquidPassword_.empty()) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG,
                 "failed to find 'password' at maethstro.liquid");
  }

  // Track config.
  for (auto& track : parser.GetConfigs("maethstro.tracks")) {
    TrackConfig trackCfg;

    const auto& name = track->Get<std::string>("name");
    if (name.empty()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "failed to find 'name' in maethstro.tracks");
    }

    auto instr = track->GetConfig("instrument");

    std::string type = instr->Get<std::string>("type");
    if (type.empty()) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "failed to find 'type' in maethstro.tracks.instrument");
    }
    if (type != kPluginTypeVst3 && type != kPluginTypeSampler) {
      RETURN_ERROR(StatusCode::INVALID_CONFIG,
                   "invalid 'type' in maethstro.tracks.instrument (not '" +
                       std::string(kPluginTypeSampler) + " or " +
                       std::string(kPluginTypeVst3) + "'sampler')");
    }
    trackCfg.instr_.type_ = type;

    // VST3 instrument config.
    if (type == kPluginTypeVst3) {
      trackCfg.instr_.vst3_.path_ = instr->Get<std::string>("path");
      if (trackCfg.instr_.vst3_.path_.empty()) {
        RETURN_ERROR(StatusCode::INVALID_CONFIG,
                     "failed to find 'path' in maethstro.tracks.instrument");
      }
      trackCfg.instr_.vst3_.preset_ = instr->Get<std::string>("preset");
      if (trackCfg.instr_.vst3_.preset_.empty()) {
        RETURN_ERROR(StatusCode::INVALID_CONFIG,
                     "failed to find 'preset' in maethstro.tracks.instrument");
      }
      LOG(INFO) << "loaded a new instrument config in maethstro config";
    }

    // Sample instrument config.
    if (type == kPluginTypeSampler) {
      trackCfg.instr_.sampler_.sampleDir_ =
          instr->Get<std::string>("sample_dir");
      if (trackCfg.instr_.sampler_.sampleDir_.empty()) {
        RETURN_ERROR(
            StatusCode::INVALID_CONFIG,
            "failed to find 'sample_dir' in maethstro.tracks.instrument");
      }

      for (auto& event : instr->GetConfigs("events")) {
        SamplerEventConfig ev;

        ev.midiNote_ = event->Get<int>("midi_note", -1);
        if (ev.midiNote_ == -1) {
          RETURN_ERROR(StatusCode::INVALID_CONFIG,
                       "failed to find 'midi_note' in "
                       "maethstro.tracks.instrument.events");
        }
        ev.midiChan_ = event->Get<int>("midi_chan", -1);
        if (ev.midiChan_ == -1) {
          RETURN_ERROR(StatusCode::INVALID_CONFIG,
                       "failed to find 'midi_chan' in "
                       "maethstro.tracks.instrument.events");
        }
        ev.sampleName_ = event->Get<std::string>("sample");
        if (ev.sampleName_.empty()) {
          RETURN_ERROR(
              StatusCode::INVALID_CONFIG,
              "failed to find 'sample' in maethstro.tracks.instrument.events");
        }

        ev.repeat_ = event->Get<bool>("repeat");

        trackCfg.instr_.sampler_.events_.push_back(ev);

        LOG(INFO) << "loaded a new event for chan=" << ev.midiChan_
                  << ", note=" << ev.midiNote_ << ", repeat=" << ev.repeat_;
      }
    }

    // FX configs.
    for (auto& effect : track->GetConfigs("effects")) {
      VST3Config fxCfg;

      fxCfg.path_ = effect->Get<std::string>("path");
      if (fxCfg.path_.empty()) {
        RETURN_ERROR(StatusCode::INVALID_CONFIG,
                     "failed to find 'path' in maethstro.tracks.effects");
      }
      fxCfg.preset_ = effect->Get<std::string>("preset");
      if (fxCfg.path_.empty()) {
        RETURN_ERROR(StatusCode::INVALID_CONFIG,
                     "failed to find 'preset' in maethstro.tracks.effects");
      }
      trackCfg.fxs_.push_back(fxCfg);
      LOG(INFO) << "loaded a new effect config in maethstro config";
    }

    LOG(INFO) << "loaded a new track config in maethstro config";
    config->tracks_.push_back(trackCfg);
  }

  LOG(INFO) << "maethstro config loaded";

  return config;
}

}  // namespace maethstro
