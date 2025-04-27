#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>
#include <absl/status/status.h>
#include <absl/time/time.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#include "utils/config.hh"
#include "utils/signal.hh"

#include "agent/agent.hh"
#include "core/soir.hh"

ABSL_FLAG(std::string, config, "etc/config.yaml", "Path to config file");
ABSL_FLAG(std::string, mode, "standalone",
          "Mode to run in (standalone, script)");
ABSL_FLAG(std::string, script, "", "Script to run in script mode");

namespace {

const std::string kModeStandalone = "standalone";
const std::string kModeScript = "script";
const std::string kVersion = "v0.9.0";
const size_t kMaxLogFiles = 10;

class FileSink : public absl::LogSink {
 public:
  explicit FileSink(const std::string& filename) {
    log_file_.open(filename, std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
      std::cerr << "Failed to open log file: " << filename << std::endl;
    }
  }

  ~FileSink() override {
    if (log_file_.is_open()) {
      log_file_.close();
    }
  }

  void Send(const absl::LogEntry& entry) override {
    if (!log_file_.is_open())
      return;

    std::string formatted_log = std::string(entry.text_message());
    std::string timestamp = absl::FormatTime(
        "%Y-%m-%d %H:%M:%S", entry.timestamp(), absl::LocalTimeZone());
    std::string severity = absl::LogSeverityName(entry.log_severity());

    log_file_ << timestamp << " " << severity << " [" << entry.source_filename()
              << ":" << entry.source_line() << "] " << formatted_log
              << std::endl;
    log_file_.flush();
  }

 private:
  std::ofstream log_file_;
};

std::unique_ptr<FileSink> file_sink;

absl::Status SetupLogDirectory() {
  char* soir_dir_env = std::getenv("SOIR_DIR");
  if (soir_dir_env == nullptr) {
    return absl::FailedPreconditionError(
        "SOIR_DIR environment variable not set");
  }

  std::filesystem::path soir_dir(soir_dir_env);
  std::filesystem::path log_dir = soir_dir / "logs";

  // Create log directory if it doesn't exist
  if (!std::filesystem::exists(log_dir)) {
    if (!std::filesystem::create_directories(log_dir)) {
      return absl::InternalError("Failed to create log directory: " +
                                 log_dir.string());
    }
  }

  // Get current time for log filename
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream date_ss;
  date_ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d-%H%M%S");
  std::string date_str = date_ss.str();

  std::filesystem::path log_file = log_dir / ("soir." + date_str + ".log");

  // Cleanup old log files if needed
  std::vector<std::filesystem::path> log_files;
  for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
    if (entry.is_regular_file() &&
        entry.path().filename().string().starts_with("soir.") &&
        entry.path().extension() == ".log") {
      log_files.push_back(entry.path());
    }
  }

  // Sort by modification time (oldest first)
  std::sort(log_files.begin(), log_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return std::filesystem::last_write_time(a) <
                     std::filesystem::last_write_time(b);
            });

  // Remove oldest files if we have more than kMaxLogFiles
  while (log_files.size() >= kMaxLogFiles) {
    std::filesystem::remove(log_files.front());
    log_files.erase(log_files.begin());
  }

  // Set up log file sink
  file_sink = std::make_unique<FileSink>(log_file.string());
  absl::AddLogSink(file_sink.get());

  return absl::OkStatus();
}

}  // namespace

namespace soir {

absl::Status Preamble() {
  LOG(INFO) << "Soir " << kVersion;
  LOG(INFO) << "█▀ █▀█ █ █▀█";
  LOG(INFO) << "▄█ █▄█ █ █▀▄ @ https://soir.dev/";
  LOG(INFO) << "";

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
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kFatal);
  absl::EnableLogPrefix(true);
  absl::InitializeLog();
  absl::SetProgramUsageMessage("soir L I V E");
  absl::ParseCommandLine(argc, argv);

  absl::Status status = SetupLogDirectory();
  if (!status.ok()) {
    std::cerr << "Failed to set up log directory: " << status << std::endl;
    return status.raw_code();
  }

  status = soir::Preamble();
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
