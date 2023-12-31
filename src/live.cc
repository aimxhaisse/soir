#include <absl/flags/usage.h>
#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <thread>

#include "common/signal.h"
#include "matin/matin.hh"
#include "midi/midi.hh"

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

absl::Status Live::StandaloneMode(const Config& config) {
  LOG(INFO) << "Running in standalone mode";

  auto midi = midi::Midi();

  auto status = midi.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize midi: " << status;
    return status;
  }

  std::thread midi_thread([&midi]() {
    auto status = midi.Run();
    if (!status.ok()) {
      LOG(ERROR) << "Unable to run midi: " << status;
      exit(1);
    }
  });

  auto matin = matin::Matin();

  status = matin.Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to initialize matin: " << status;
    return status;
  }

  status = matin.Start();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to start matin: " << status;
    return status;
  }

  status = WaitForExitSignal();
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

  status = midi.Wait();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for midi: " << status;
    return status;
  }

  midi_thread.join();

  return absl::OkStatus();
}

absl::Status Live::MatinMode(const Config& config) {
  LOG(ERROR) << "Matin mode not yet implemented";

  return absl::UnimplementedError("Matin mode not yet implemented");
}

absl::Status Live::MidiMode(const Config& config) {
  LOG(ERROR) << "Midi mode not yet implemented";

  return absl::UnimplementedError("Midi mode not yet implemented");
}

absl::Status Live::SoirMode(const Config& config) {
  LOG(ERROR) << "Soir mode not yet implemented";

  return absl::UnimplementedError("Soir mode not yet implemented");
}

}  // namespace maethstro
