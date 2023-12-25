#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ConfigParser.hh"
#include "Status.hh"

namespace maethstro {

// Configuration of a sampler event.
struct SamplerEventConfig {
  int midiNote_;
  int midiChan_;
  bool repeat_ = false;
  bool stretch_ = false;
  float stretchDuration_ = 8.0;
  std::string sampleName_;
};

// Configuration of a sampler.
struct SamplerConfig {
  std::string sampleDir_;
  std::vector<SamplerEventConfig> events_;
};

// Configuration of a VST3 instrument.
struct VST3Config {
  std::string path_;
  std::string preset_;
};

// Configuration of an instrument (VST3).
struct InstrumentConfig {
  std::string type_;

  // Depending on the type of the instrument, one of those structures
  // will be filled accordingly.
  VST3Config vst3_;
  SamplerConfig sampler_;
};

// Configuration of a track.
struct TrackConfig {
  std::string name_;
  InstrumentConfig instr_;
  std::vector<VST3Config> fxs_;
};

// Configuration of Maethstro.
class Config {
 public:
  static StatusOr<std::unique_ptr<Config>> LoadFromParser(ConfigParser& parser);

  int port_;
  std::string host_;
  int bpm_;

  int liquidPort_;
  std::string liquidHost_;
  std::string liquidMount_;
  std::string liquidPassword_;

  std::vector<TrackConfig> tracks_;
};

}  // namespace maethstro
