#pragma once

#include <absl/status/status.h>
#include <efsw/efsw.hpp>
#include <regex>
#include <thread>

#include "soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {
namespace agent {

class FileWatcher : efsw::FileWatchListener {
 public:
  FileWatcher();
  ~FileWatcher();

  absl::Status Init(const utils::Config& config, proto::Soir::Stub* stub);
  absl::Status Start();
  absl::Status Stop();

  // Returns true if the filename matches the pattern for a live
  // coding file from the watched directory.
  bool IsLiveCodingFile(const std::string& filename) const;

  // Sends the content of all live coding files at start-up to Soir via gRPC.
  absl::Status InitialCodeUpdate() const;

  // Sends the content of the updated file to Soir via gRPC.
  absl::Status SendCodeUpdate(const std::string& filename) const;

 private:
  // Callback for efsw::FileWatchListener.
  void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                        const std::string& filename, efsw::Action action,
                        std::string old_filename) override;

  std::thread thread_;

  std::string directory_;

  proto::Soir::Stub* soir_stub_;
  std::unique_ptr<efsw::FileWatcher> file_watcher_;
  std::regex file_pattern_;
};

}  // namespace agent
}  // namespace soir
