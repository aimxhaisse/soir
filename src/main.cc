#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/status/status.h>

#include "live.hh"

ABSL_FLAG(std::string, config, "etc/standalone.yaml", "Path to config file");
ABSL_FLAG(std::string, mode, "standalone",
          "Mode to run in (standalone, matin, midi, soir)");

namespace {

const std::string kModeStandalone = "standalone";
const std::string kModeMatin = "matin";
const std::string kModeMidi = "midi";
const std::string kModeSoir = "soir";

}  // namespace

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
  auto config = maethstro::Config::LoadFromPath(config_file);
  if (!config.ok()) {
    LOG(ERROR) << "Failed to load configuration file: " << config_file;
    return config.status().raw_code();
  }

  std::string mode = absl::GetFlag(FLAGS_mode);
  if (mode == kModeStandalone) {
    status = maethstro::Live::Standalone();
  } else if (mode == kModeMatin) {
    status = maethstro::Live::Matin();
  } else if (mode == kModeMidi) {
    status = maethstro::Live::Midi();
  } else if (mode == kModeSoir) {
    status = maethstro::Live::Soir();
  } else {
    LOG(ERROR) << "Unknown mode: " << mode;
    return static_cast<int>(absl::StatusCode::kInvalidArgument);
  }

  return absl::OkStatus().raw_code();
}
