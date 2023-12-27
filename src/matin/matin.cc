#include <absl/log/log.h>

#include "matin.hh"

namespace maethstro {

Matin::Matin() {}

Matin::~Matin() {}

absl::Status Matin::Init(const Config& config) {
  settings_.directory = config.Get<std::string>("directory");
  settings_.midi_grpc_host = config.Get<std::string>("midi_grpc_host");
  settings_.midi_grpc_port = config.Get<int>("midi_grpc_port");

  LOG(INFO) << "Matin initialized with settings: " << settings_.directory
            << ", " << settings_.midi_grpc_host << ", "
            << settings_.midi_grpc_port;

  return absl::OkStatus();
}

absl::Status Matin::Run() {
  return absl::OkStatus();
}

}  // namespace maethstro
