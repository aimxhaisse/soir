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

  absl::Status Init(const std::string& log_dir, size_t max_files = 25);
  absl::Status Shutdown();

  bool IsInitialized() const { return initialized_; }

 private:
  Logger() = default;
  ~Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  bool initialized_ = false;
  std::unique_ptr<FileSink> file_sink_;
};

}  // namespace utils
}  // namespace soir

#endif  // SOIR_UTILS_LOGGER_HH_
