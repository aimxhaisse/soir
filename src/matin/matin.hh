#pragma once

#include <absl/status/status.h>
#include <efsw/efsw.hpp>

#include "common/config.hh"

namespace maethstro {

struct MatinSettings {
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

  void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                        const std::string& filename, efsw::Action action,
                        std::string oldFilename) override;

 private:
  MatinSettings settings_;

  std::unique_ptr<efsw::FileWatcher> file_watcher_;
};

}  // namespace maethstro
