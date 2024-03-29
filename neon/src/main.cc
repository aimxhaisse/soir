#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/status/status.h>

#include "utils/config.hh"
#include "utils/signal.hh"

#include "agent/agent.hh"
#include "core/neon.hh"

ABSL_FLAG(std::string, config, "etc/standalone.yaml", "Path to config file");
ABSL_FLAG(std::string, mode, "standalone", "Mode to run in (standalone)");

namespace {

const std::string kModeStandalone = "standalone";
const std::string kVersion = "0.0.2-alpha.1";

}  // namespace

namespace neon {

absl::Status Preamble() {

  LOG(INFO) << "Neon v" << kVersion;
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

absl::Status StandaloneMode(const utils::Config& config) {
  LOG(INFO) << "Running in standalone mode";

  std::unique_ptr<Neon> neon = std::make_unique<Neon>();

  absl::Status status = neon->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize Neon: " << status;
    return status;
  }

  LOG(INFO) << "Starting Neon";

  status = neon->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start Neon: " << status;
    return status;
  }

  auto agent = std::make_unique<agent::Agent>();

  status = agent->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize Agent: " << status;
    return status;
  }

  LOG(INFO) << "Neon is running";

  status = utils::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  LOG(INFO) << "Stopping Neon";

  status = neon->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop Neon: " << status;
    return status;
  }

  return absl::OkStatus();
}

}  // namespace neon

int main(int argc, char* argv[]) {
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::EnableLogPrefix(true);
  absl::InitializeLog();

  absl::SetProgramUsageMessage("Maethstro L I V E");
  absl::ParseCommandLine(argc, argv);

  absl::Status status = neon::Preamble();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to run neon preamble: " << status;
    return status.raw_code();
  }

  std::string config_file = absl::GetFlag(FLAGS_config);
  auto config = neon::utils::Config::LoadFromPath(config_file);
  if (!config.ok()) {
    LOG(ERROR) << "Failed to load configuration file: " << config_file;
    return config.status().raw_code();
  }

  std::string mode = absl::GetFlag(FLAGS_mode);
  if (mode == kModeStandalone) {
    status = neon::StandaloneMode(**config);
  } else {
    LOG(ERROR) << "Unknown mode: " << mode;
    return static_cast<int>(absl::StatusCode::kInvalidArgument);
  }

  return absl::OkStatus().raw_code();
}
