#include <absl/log/log.h>
#include <absl/status/status.h>
#include <filesystem>

#include "common.hh"
#include "track.hh"

namespace maethstro {
namespace soir {

Track::Track() {}

absl::Status Track::Init(const common::Config& config) {
  channel_ = config.Get<int>("channel");

  auto instrument = config.Get<std::string>("instrument");
  if (instrument != "sampler") {
    return absl::InvalidArgumentError("Only sampler instrument is supported");
  }
  sampler_ = std::make_unique<MonoSampler>();

  auto status = sampler_->Init(config, channel_);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to init sampler: " << status.message();
    return status;
  }

  return absl::OkStatus();
}

void Track::Render(const std::list<proto::MidiEvents_Request>& midi_events,
                   AudioBuffer& buffer) {
  sampler_->Render(midi_events, buffer);
}

}  // namespace soir
}  // namespace maethstro
