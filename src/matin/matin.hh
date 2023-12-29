#pragma once

#include <absl/status/status.h>
#include <efsw/efsw.hpp>
#include <regex>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {

struct MatinSettings {
  std::string username;
  std::string directory;
  std::string midi_grpc_host;
  int midi_grpc_port;
};

class Matin : efsw::FileWatchListener {
 public:
  Matin();
  ~Matin();

  absl::Status Init(const Config& config);
  absl::Status Run();
  absl::Status Stop();
  absl::Status Wait();

  // Returns true if the filename matches the pattern for a live
  // coding file from the watched directory.
  bool IsLiveCodingFile(const std::string& filename) const;

  // Sends the content of all live coding files at start-up to Midi via gRPC.
  absl::Status InitialLiveUpdate() const;

  // Sends the content of the updated file to Midi via gRPC.
  absl::Status LiveUpdate(const std::string& filename) const;

 private:
  // Callback for efsw::FileWatchListener.
  void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                        const std::string& filename, efsw::Action action,
                        std::string old_filename) override;

  MatinSettings settings_;
  std::unique_ptr<proto::Midi::Stub> midi_stub_;
  std::unique_ptr<efsw::FileWatcher> file_watcher_;
  std::regex file_pattern_;
};

}  // namespace maethstro
