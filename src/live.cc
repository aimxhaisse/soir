#include <absl/flags/usage.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>

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

absl::Status Live::Standalone() {
  LOG(INFO) << "Running in standalone mode";

  return absl::OkStatus();
}

absl::Status Live::Matin() {
  LOG(ERROR) << "matin mode not yet implemented";

  return absl::UnimplementedError("matin mode not yet implemented");
}

absl::Status Live::Midi() {
  LOG(ERROR) << "midi mode not yet implemented";

  return absl::UnimplementedError("midi mode not yet implemented");
}

absl::Status Live::Soir() {
  LOG(ERROR) << "soir mode not yet implemented";

  return absl::UnimplementedError("soir mode not yet implemented");
}

}  // namespace maethstro
