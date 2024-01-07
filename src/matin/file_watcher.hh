#pragma once

#include <absl/status/status.h>
#include <efsw/efsw.hpp>
#include <regex>
#include <thread>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace matin {

class FileWatcher : efsw::FileWatchListener {
 public:
  FileWatcher();
  ~FileWatcher();

  absl::Status Init(const common::Config& config, proto::Midi::Stub* stub);
  absl::Status Start();
  absl::Status Stop();

  // Returns true if the filename matches the pattern for a live
  // coding file from the watched directory.
  bool IsLiveCodingFile(const std::string& filename) const;

  // Sends the content of all live coding files at start-up to Midi via gRPC.
  absl::Status InitialCodeUpdate() const;

  // Sends the content of the updated file to Midi via gRPC.
  absl::Status SendCodeUpdate(const std::string& filename) const;

 private:
  // Callback for efsw::FileWatchListener.
  void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                        const std::string& filename, efsw::Action action,
                        std::string old_filename) override;

  std::thread thread_;

  std::string user_;
  std::string directory_;

  proto::Midi::Stub* midi_stub_;
  std::unique_ptr<efsw::FileWatcher> file_watcher_;
  std::regex file_pattern_;
};

}  // namespace matin
}  // namespace maethstro
