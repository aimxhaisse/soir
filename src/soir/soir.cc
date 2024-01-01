#include "soir.hh"

namespace maethstro {
namespace soir {

Soir::Soir() {}

Soir::~Soir() {}

absl::Status Soir::Init(const common::Config& config) {
  return absl::OkStatus();
}

absl::Status Soir::Start() {
  return absl::OkStatus();
}

absl::Status Soir::Stop() {
  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
