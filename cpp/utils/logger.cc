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
#include <random>
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

  std::lock_guard<std::mutex> lock(mutex_);
  log_file_ << timestamp << " " << severity << " [" << source_file << ":"
            << entry.source_line() << "] " << formatted_log << std::endl;
  log_file_.flush();
}

Logger& Logger::Instance() {
  static Logger instance;
  return instance;
}

std::string Logger::MakeTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d-%H%M%S");
  return ss.str();
}

std::string Logger::GenerateUid() {
  std::random_device rd;
  std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(8) << dist(rd)
     << std::setw(8) << dist(rd);
  return ss.str();
}

// File-scope helper: removes all files in log_dir whose name starts with
// prefix and ends with ".log", keeping at most max_files (oldest first).
static void RotateByPrefix(const std::filesystem::path& log_dir,
                           const std::string& prefix, size_t max_files) {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
    auto name = entry.path().filename().string();
    if (entry.is_regular_file() && name.rfind(prefix, 0) == 0 &&
        entry.path().extension() == ".log") {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return std::filesystem::last_write_time(a) <
                     std::filesystem::last_write_time(b);
            });
  while (files.size() >= max_files) {
    std::filesystem::remove(files.front());
    files.erase(files.begin());
  }
}

void Logger::RotateEngineLogFiles(const std::string& log_dir,
                                  size_t max_files) {
  std::filesystem::path log_path(log_dir);
  constexpr std::string_view kEnginePrefix = "soir.engine.";

  std::vector<std::filesystem::path> engine_files;
  for (const auto& entry : std::filesystem::directory_iterator(log_path)) {
    auto name = entry.path().filename().string();
    if (entry.is_regular_file() && name.rfind(kEnginePrefix, 0) == 0 &&
        entry.path().extension() == ".log") {
      engine_files.push_back(entry.path());
    }
  }

  std::sort(engine_files.begin(), engine_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return std::filesystem::last_write_time(a) <
                     std::filesystem::last_write_time(b);
            });

  while (engine_files.size() >= max_files) {
    // Extract the uid from "soir.engine.{uid}.{ts}.log" — the segment
    // immediately after the "soir.engine." prefix, before the next dot.
    std::string name = engine_files.front().filename().string();
    std::string rest = name.substr(kEnginePrefix.size());
    std::string uid = rest.substr(0, rest.find('.'));

    // Cascade: remove all VST log files for this session.
    if (!uid.empty()) {
      for (const auto& entry : std::filesystem::directory_iterator(log_path)) {
        auto vst_name = entry.path().filename().string();
        if (entry.is_regular_file() && vst_name.rfind("soir.vst.", 0) == 0 &&
            vst_name.find("." + uid + ".") != std::string::npos &&
            entry.path().extension() == ".log") {
          std::filesystem::remove(entry.path());
        }
      }
    }

    std::filesystem::remove(engine_files.front());
    engine_files.erase(engine_files.begin());
  }
}

absl::Status Logger::Init(const std::string& log_dir, size_t max_files,
                          bool verbose, bool redirect_stdio) {
  if (initialized_) {
    if (file_sink_) {
      absl::RemoveLogSink(file_sink_.get());
      file_sink_.reset();
    }
  }

  if (verbose) {
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  } else {
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kFatal);
  }

  std::filesystem::path log_path(log_dir);

  if (!std::filesystem::exists(log_path)) {
    if (!std::filesystem::create_directories(log_path)) {
      return absl::InternalError("Failed to create log directory: " +
                                 log_path.string());
    }
  }

  log_dir_ = log_dir;
  max_files_ = max_files;
  session_uid_ = GenerateUid();

  RotateEngineLogFiles(log_dir, max_files);

  std::filesystem::path log_file = log_path / ("soir.engine." + session_uid_ +
                                               "." + MakeTimestamp() + ".log");

  if (redirect_stdio) {
    freopen(log_file.string().c_str(), "a", stdout);
    freopen(log_file.string().c_str(), "a", stderr);
  }

  file_sink_ = std::make_unique<FileSink>(log_file.string());
  absl::AddLogSink(file_sink_.get());

  std::filesystem::path latest_link = log_path / "latest.log";
  std::error_code ec;
  std::filesystem::remove(latest_link, ec);
  std::filesystem::create_symlink(log_file.filename(), latest_link, ec);
  if (ec) {
    std::cerr << "Failed to create latest.log symlink: " << ec.message()
              << std::endl;
  }

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
