#include "neon.hh"

namespace neon {

Neon::Neon() : dsp_(std::make_unique<dsp::Engine>()) {}

Neon::~Neon() {}

absl::Status Neon::Init(const utils::Config& config) {
  return dsp_->Init(config);
}

absl::Status Neon::Start() {
  return dsp_->Start();
}

absl::Status Neon::Stop() {
  return dsp_->Stop();
}

}  // namespace neon
