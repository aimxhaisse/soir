#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/status/status.h>

#include "common/config.hh"
#include "common/signal.hh"
#include "matin/matin.hh"
#include "midi/midi.hh"
#include "soir/soir.hh"

ABSL_FLAG(std::string, config, "etc/standalone.yaml", "Path to config file");
ABSL_FLAG(std::string, mode, "standalone",
          "Mode to run in (standalone, matin, midi, soir)");

namespace {

const std::string kModeStandalone = "standalone";
const std::string kModeMatin = "matin";
const std::string kModeMidi = "midi";
const std::string kModeSoir = "soir";

constexpr const char* kVersion = "0.0.1-alpha.1";

}  // namespace

namespace maethstro {

class Live {
 public:
  static absl::Status Preamble();

  static absl::Status StandaloneMode(const common::Config& config);
  static absl::Status MatinMode(const common::Config& config);
  static absl::Status MidiMode(const common::Config& config);
  static absl::Status SoirMode(const common::Config& config);
};

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

int main(int argc, char* argv[]) {
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::EnableLogPrefix(true);
  absl::InitializeLog();

  absl::SetProgramUsageMessage("Maethstro L I V E");
  absl::ParseCommandLine(argc, argv);

  absl::Status status = maethstro::Live::Preamble();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to run Maethstro preamble: " << status;
    return status.raw_code();
  }

  std::string config_file = absl::GetFlag(FLAGS_config);
  auto config = maethstro::common::Config::LoadFromPath(config_file);
  if (!config.ok()) {
    LOG(ERROR) << "Failed to load configuration file: " << config_file;
    return config.status().raw_code();
  }

  std::string mode = absl::GetFlag(FLAGS_mode);
  if (mode == kModeStandalone) {
    status = maethstro::Live::StandaloneMode(**config);
  } else if (mode == kModeMatin) {
    status = maethstro::Live::MatinMode(**config);
  } else if (mode == kModeMidi) {
    status = maethstro::Live::MidiMode(**config);
  } else if (mode == kModeSoir) {
    status = maethstro::Live::SoirMode(**config);
  } else {
    LOG(ERROR) << "Unknown mode: " << mode;
    return static_cast<int>(absl::StatusCode::kInvalidArgument);
  }

  return absl::OkStatus().raw_code();
}
