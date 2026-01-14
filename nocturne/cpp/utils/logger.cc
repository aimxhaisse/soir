#include "utils/logger.hh"

#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/log/log_sink_registry.h>
#include <absl/time/time.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

namespace soir {
namespace utils {

FileSink::FileSink(const std::string& filename) {
  log_file_.open(filename, std::ios::out | std::ios::app);
  if (!log_file_.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
  }
}

FileSink::~FileSink() {
  if (log_file_.is_open()) {
    log_file_.close();
  }
}

void FileSink::Send(const absl::LogEntry& entry) {
  if (!log_file_.is_open()) {
    return;
  }

  std::string formatted_log = std::string(entry.text_message());
  std::string timestamp = absl::FormatTime(
      "%Y-%m-%d %H:%M:%S", entry.timestamp(), absl::LocalTimeZone());
  std::string severity = absl::LogSeverityName(entry.log_severity());

  std::filesystem::path source_path(entry.source_filename());
  std::string source_file = source_path.filename().string();

  log_file_ << timestamp << severity << " [" << source_file << ":"
            << entry.source_line() << "] " << formatted_log << std::endl;
  log_file_.flush();
}

Logger& Logger::Instance() {
  static Logger instance;
  return instance;
}

absl::Status Logger::Init(const std::string& log_dir, size_t max_files,
                          bool verbose) {
  if (initialized_) {
    if (file_sink_) {
      absl::RemoveLogSink(file_sink_.get());
      file_sink_.reset();
    }
  }

  // Set stderr threshold based on verbose flag
  if (verbose) {
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  } else {
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kFatal);
  }

  std::filesystem::path log_path(log_dir);

  // Create log directory if it doesn't exist
  if (!std::filesystem::exists(log_path)) {
    if (!std::filesystem::create_directories(log_path)) {
      return absl::InternalError("Failed to create log directory: " +
                                 log_path.string());
    }
  }

  // Get current time for log filename
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream date_ss;
  date_ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d-%H%M%S");
  std::string date_str = date_ss.str();

  std::filesystem::path log_file = log_path / ("soir." + date_str + ".log");

  // Cleanup old log files if needed
  std::vector<std::filesystem::path> log_files;
  for (const auto& entry : std::filesystem::directory_iterator(log_path)) {
    auto filename = entry.path().filename().string();
    if (entry.is_regular_file() && filename.rfind("soir.", 0) == 0 &&
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

  // Remove oldest files if we have more than max_files
  while (log_files.size() >= max_files) {
    std::filesystem::remove(log_files.front());
    log_files.erase(log_files.begin());
  }

  // Set up log file sink
  file_sink_ = std::make_unique<FileSink>(log_file.string());
  absl::AddLogSink(file_sink_.get());

  if (!initialized_) {
    absl::InitializeLog();
    initialized_ = true;
  }

  LOG(INFO) << "Logger initialized: " << log_file.string();

  return absl::OkStatus();
}

absl::Status Logger::Shutdown() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Logger not initialized");
  }

  if (file_sink_) {
    absl::RemoveLogSink(file_sink_.get());
    file_sink_.reset();
  }

  return absl::OkStatus();
}

}  // namespace utils
}  // namespace soir
