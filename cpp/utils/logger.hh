#ifndef SOIR_UTILS_LOGGER_HH_
#define SOIR_UTILS_LOGGER_HH_

#include <fstream>
#include <memory>
#include <string>

#include "absl/log/log_sink.h"
#include "absl/status/status.h"

namespace soir {
namespace utils {

class FileSink : public absl::LogSink {
 public:
  explicit FileSink(const std::string& filename);
  ~FileSink() override;

  void Send(const absl::LogEntry& entry) override;

 private:
  std::ofstream log_file_;
};

class Logger {
 public:
  static Logger& Instance();

  absl::Status Init(const std::string& log_dir, size_t max_files = 25,
                    bool verbose = false, bool redirect_stdio = false);
  absl::Status Shutdown();

  bool IsInitialized() const { return initialized_; }
  const std::string& GetLogDir() const { return log_dir_; }
  const std::string& GetSessionUid() const { return session_uid_; }

  // Returns a timestamp string in the format "YYYYMMDD-HHMMSS".
  static std::string MakeTimestamp();

 private:
  Logger() = default;
  ~Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  static std::string GenerateUid();
  static void RotateEngineLogFiles(const std::string& log_dir,
                                   size_t max_files);

  bool initialized_ = false;
  std::string log_dir_;
  std::string session_uid_;
  size_t max_files_ = 25;
  std::unique_ptr<FileSink> file_sink_;
};

}  // namespace utils
}  // namespace soir

#endif  // SOIR_UTILS_LOGGER_HH_
