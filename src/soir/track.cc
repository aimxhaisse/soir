#include <absl/log/log.h>
#include <absl/status/status.h>

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

  auto directory = config.Get<std::string>("sample_dir");

  // TODO: here load samples.

  LOG(INFO) << "Loaded track " << channel_ << " with " << samples_.size()
            << " samples";

  return absl::OkStatus();
}

void Track::Render(AudioBuffer& buffer) {}

}  // namespace soir
}  // namespace maethstro
