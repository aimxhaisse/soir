#include <absl/flags/usage.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>

#include "common/signal.h"

#include "live.hh"

namespace maethstro {

absl::Status Live::Preamble() {

  LOG(INFO) << "Maethstro L I V E " << std::string(kVersion);
  LOG(INFO) << "";
  LOG(INFO) << " _       ___  __     __  _____";
  LOG(INFO) << "| |     |_ _| \\ \\   / / | ____|";
  LOG(INFO) << "| |      | |   \\ \\ / /  |  _|  ";
  LOG(INFO) << "| |___   | |    \\ V /   | |___ ";
  LOG(INFO) << "|_____| |___|    \\_/    |_____|";
  LOG(INFO) << "";
  LOG(INFO) << "Happy c0ding!";

  return absl::OkStatus();
}

absl::Status Live::Standalone(const Config& config) {
  LOG(INFO) << "Running in standalone mode";

  auto status = WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Live::Matin(const Config& config) {
  LOG(ERROR) << "Matin mode not yet implemented";

  return absl::UnimplementedError("Matin mode not yet implemented");
}

absl::Status Live::Midi(const Config& config) {
  LOG(ERROR) << "Midi mode not yet implemented";

  return absl::UnimplementedError("Midi mode not yet implemented");
}

absl::Status Live::Soir(const Config& config) {
  LOG(ERROR) << "Soir mode not yet implemented";

  return absl::UnimplementedError("Soir mode not yet implemented");
}

}  // namespace maethstro
