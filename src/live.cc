#include <absl/flags/usage.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>

#include "common/signal.hh"
#include "matin/matin.hh"
#include "midi/midi.hh"
#include "soir/soir.hh"

#include "live.hh"

namespace maethstro {

absl::Status Live::Preamble() {

  LOG(INFO) << "Maethstro v" << std::string(kVersion);
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

absl::Status Live::StandaloneMode(const common::Config& config) {
  LOG(INFO) << "Running in standalone mode";

  auto soir = soir::Soir();
  auto status = soir.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize soir: " << status;
    return status;
  }

  auto midi = midi::Midi();
  status = midi.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start midi: " << status;
    return status;
  }

  auto matin = matin::Matin();
  status = matin.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize matin: " << status;
    return status;
  }

  status = soir.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start soir: " << status;
    return status;
  }

  status = midi.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start midi: " << status;
    return status;
  }

  status = matin.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start matin: " << status;
    return status;
  }

  status = common::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  status = matin.Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to stop matin: " << status;
    return status;
  }

  status = midi.Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to stop midi: " << status;
    return status;
  }

  status = soir.Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to stop soir: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Live::MatinMode(const common::Config& config) {
  LOG(INFO) << "Running in Matin mode";

  auto matin = matin::Matin();
  auto status = matin.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize matin: " << status;
    return status;
  }

  status = matin.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start matin: " << status;
    return status;
  }

  status = common::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  status = matin.Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to stop matin: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Live::MidiMode(const common::Config& config) {
  LOG(INFO) << "Running in Midi mode";

  auto midi = midi::Midi();
  auto status = midi.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize midi: " << status;
    return status;
  }

  status = midi.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start midi: " << status;
    return status;
  }

  status = common::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  status = midi.Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to stop midi: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status Live::SoirMode(const common::Config& config) {
  LOG(ERROR) << "Soir mode not yet implemented";

  return absl::UnimplementedError("Soir mode not yet implemented");
}

}  // namespace maethstro
