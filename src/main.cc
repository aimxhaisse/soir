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
#include "core/soir.hh"

ABSL_FLAG(std::string, config, "etc/standalone.yaml", "Path to config file");
ABSL_FLAG(std::string, mode, "standalone",
          "Mode to run in (standalone, script)");
ABSL_FLAG(std::string, script, "", "Script to run in script mode");

namespace {

const std::string kModeStandalone = "standalone";
const std::string kModeScript = "script";
const std::string kVersion = "v0.1.0";

}  // namespace

namespace soir {

absl::Status Preamble() {
  LOG(INFO) << "Soir v" << kVersion;
  LOG(INFO) << "█▀ █▀█ █ █▀█";
  LOG(INFO) << "▄█ █▄█ █ █▀▄";
  LOG(INFO) << "";
  LOG(INFO) << "Happy c0ding!";

  return absl::OkStatus();
}

absl::Status StandaloneMode(const utils::Config& config) {
  LOG(INFO) << "Running in standalone mode";

  std::unique_ptr<Soir> soir = std::make_unique<Soir>();

  absl::Status status = soir->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize Soir: " << status;
    return status;
  }

  LOG(INFO) << "Starting Soir";

  status = soir->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start Soir: " << status;
    return status;
  }

  auto agent = std::make_unique<agent::Agent>();

  status = agent->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize Agent: " << status;
    return status;
  }

  status = agent->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start Agent: " << status;
    return status;
  }

  LOG(INFO) << "Soir is running";

  status = utils::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    return status;
  }

  LOG(INFO) << "Stopping Soir";

  status = agent->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop Agent: " << status;
    return status;
  }

  status = soir->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop Soir: " << status;
    return status;
  }

  return absl::OkStatus();
}

absl::Status ScriptMode(const utils::Config& config,
                        const std::string& script_path) {
  LOG(INFO) << "Running in standalone mode";

  std::unique_ptr<Soir> soir = std::make_unique<Soir>();

  absl::Status status = soir->Init(config);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize Soir: " << status;
    return status;
  }

  LOG(INFO) << "Starting Soir";

  status = soir->Start();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to start Soir: " << status;
    return status;
  }

  auto script_or = utils::GetFileContents(script_path);
  if (!script_or.ok()) {
    LOG(ERROR) << "Failed to load script: " << script_or.status();
    soir->Stop().IgnoreError();
    return script_or.status();
  }

  // Have the script exit after running, this will trigger a graceful
  // shutdown via WaitForExitSignal.
  auto script = script_or.value() + "\nraise SystemExit()\n";

  proto::PushCodeUpdateRequest request;
  request.set_code(script);
  auto grpc_status = soir->PushCodeUpdate(nullptr, &request, nullptr);
  if (!grpc_status.ok()) {
    LOG(ERROR) << "Failed to push script: " << grpc_status.error_message();
    soir->Stop().IgnoreError();
    return absl::InternalError("Failed to push script");
  }

  status = utils::WaitForExitSignal();
  if (!status.ok()) {
    LOG(ERROR) << "Unable to wait for exit signal: " << status;
    soir->Stop().IgnoreError();
    return status;
  }

  LOG(INFO) << "Stopping Soir";

  status = soir->Stop();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to stop Soir: " << status;
    return status;
  }

  return absl::OkStatus();
}

}  // namespace soir

int main(int argc, char* argv[]) {
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::EnableLogPrefix(true);
  absl::InitializeLog();
  absl::SetProgramUsageMessage("Maethstro L I V E");
  absl::ParseCommandLine(argc, argv);

  absl::Status status = soir::Preamble();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to run soir preamble: " << status;
    return status.raw_code();
  }

  std::string config_file = absl::GetFlag(FLAGS_config);
  auto config = soir::utils::Config::LoadFromPath(config_file);
  if (!config.ok()) {
    LOG(ERROR) << "Failed to load configuration file: " << config_file;
    return config.status().raw_code();
  }

  std::string mode = absl::GetFlag(FLAGS_mode);
  if (mode == kModeStandalone) {
    status = soir::StandaloneMode(**config);
  } else if (mode == kModeScript) {
    status = soir::ScriptMode(**config, absl::GetFlag(FLAGS_script));

  } else {
    LOG(ERROR) << "Unknown mode: " << mode;
    return static_cast<int>(absl::StatusCode::kInvalidArgument);
  }

  return absl::OkStatus().raw_code();
}
